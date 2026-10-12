#include <iostream>
#include <string>
#include <cstring>
#include <sqlite3.h>
#include <cstdio>
#include <libmemcached/memcached.h>
#include <chrono>
#include <thread>
#include <mosquitto.h>
#include "mongoose.h"

struct AppConfig {
    std::string db_path;
    std::string slave1_url;
    std::string slave2_url;
};

AppConfig config;
memcached_st *memc;


std::string query_local_db(const std::string& type, const std::string& id) {
    sqlite3* db;
    if (sqlite3_open(config.db_path.c_str(), &db) != SQLITE_OK) return "";
    std::string query = "SELECT r.value, s.unit FROM sensors s JOIN sensor_readings r ON s.sensor_id = r.sensor_id WHERE s.sensor_type = ? AND s.sensor_id = ? ORDER BY r.recorded_at DESC LIMIT 1;";
    sqlite3_stmt* stmt;
    std::string result = "";
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string unit = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            result = "{\"value\": " + val + ", \"unit\": \"" + unit + "\"}";
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

std::string fetch_from_slave(const std::string& base_url, const std::string& type, const std::string& id) {
    std::string result = "";
    char buffer[256];
    std::string cmd = "curl -s -m 2 \"" + base_url + "/api/sensor?type=" + type + "&id=" + id + "\"";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) result += buffer;
    pclose(pipe);
    return result;
}

std::string extract_data_from_json(const std::string& json_str) {
    size_t data_pos = json_str.find("\"data\": {");
    if (data_pos == std::string::npos) return "";
    size_t start = data_pos + 8;
    size_t end = json_str.find("}", start);
    if (end == std::string::npos) return "";
    return json_str.substr(start, end - start + 1);
}

std::string process_request_with_cache(const std::string& type, const std::string& id) {
    auto start_time = std::chrono::high_resolution_clock::now();
    std::string cache_key = "master_" + type + "_" + id;
    memcached_return_t rc;
    size_t value_length;
    uint32_t flags;

    char* cache_result = memcached_get(memc, cache_key.c_str(), cache_key.length(), &value_length, &flags, &rc);
    if (rc == MEMCACHED_SUCCESS && cache_result != nullptr) {
        std::string val(cache_result);
        free(cache_result);
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;
        return "{\"status\": \"success\", \"data\": " + val + ", \"source\": \"Master_Memcached\", \"node\": \"Master\", \"response_time_ms\": " + std::to_string(duration.count()) + "}";
    }

    std::string found_data = query_local_db(type, id);
    std::string source_node = "Master";
    std::string source_type = "SQLite";

    if (found_data.empty()) {
        std::string s1_res = fetch_from_slave(config.slave1_url, type, id);
        found_data = extract_data_from_json(s1_res);
        if (!found_data.empty()) { source_node = "Slave1"; source_type = "Network"; }
    }

    if (found_data.empty()) {
        std::string s2_res = fetch_from_slave(config.slave2_url, type, id);
        found_data = extract_data_from_json(s2_res);
        if (!found_data.empty()) { source_node = "Slave2"; source_type = "Network"; }
    }

    if (found_data.empty()) return "{\"error\": \"Sensor data not found\"}";

    memcached_set(memc, cache_key.c_str(), cache_key.length(), found_data.c_str(), found_data.length(), 60, 0);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end_time - start_time;
    
    return "{\"status\": \"success\", \"data\": " + found_data + ", \"source\": \"" + source_type + "\", \"node\": \"" + source_node + "\", \"response_time_ms\": " + std::to_string(duration.count()) + "}";
}

// --- MQTT connection and message-processing functions ---
void on_mqtt_connect(struct mosquitto *mosq, void *obj, int reason_code) {
    if(reason_code == 0) {
        // Subscribe Subscribe to sensor requests at QoS 1
        mosquitto_subscribe(mosq, NULL, "hotel/sensors/req/+/+", 1);
        std::cout << "[MQTT] Connected to Broker successfully. Listening on 'hotel/sensors/req/+/+'\n";
    }
}

void on_mqtt_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    std::string topic(msg->topic);
    

    size_t first_slash = topic.find_last_of('/', topic.find_last_of('/') - 1);
    size_t second_slash = topic.find_last_of('/');
    
    if (first_slash != std::string::npos && second_slash != std::string::npos) {
        std::string type = topic.substr(first_slash + 1, second_slash - first_slash - 1);
        std::string id = topic.substr(second_slash + 1);

 
        std::string json_response = process_request_with_cache(type, id);

   
        std::string res_topic = "hotel/sensors/res/" + type + "/" + id;
        
   
        mosquitto_publish(mosq, NULL, res_topic.c_str(), json_response.length(), json_response.c_str(), 1, false);
    }
}

void mqtt_worker() {
    mosquitto_lib_init();
    struct mosquitto *mosq = mosquitto_new("master_mqtt_node", true, NULL);
    mosquitto_connect_callback_set(mosq, on_mqtt_connect);
    mosquitto_message_callback_set(mosq, on_mqtt_message);
    
 
    if(mosquitto_connect(mosq, "127.0.0.1", 1883, 60) == MOSQ_ERR_SUCCESS) {
        mosquitto_loop_forever(mosq, -1, 1);
    }
    
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}


static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        if (hm->uri.len >= 11 && strncmp(hm->uri.buf, "/api/sensor", 11) == 0) {
            char type[50] = {0}, id[50] = {0};
            mg_http_get_var(&hm->query, "type", type, sizeof(type));
            mg_http_get_var(&hm->query, "id", id, sizeof(id));
            if (strlen(type) > 0 && strlen(id) > 0) {
                std::string json_response = process_request_with_cache(type, id);
                if(json_response.find("error") != std::string::npos) mg_http_reply(c, 404, "Content-Type: application/json\r\n", "%s", json_response.c_str());
                else mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json_response.c_str());
            } else {
                mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\": \"Missing parameters\"}");
            }
        } else {
            mg_http_reply(c, 404, "Content-Type: application/json\r\n", "{\"error\": \"Endpoint not found\"}");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <PORT> <DB_PATH> <SLAVE1_IP:PORT> <SLAVE2_IP:PORT>\n";
        return 1;
    }
    std::string port = argv[1];
    config.db_path = argv[2];
    config.slave1_url = "http://" + std::string(argv[3]);
    config.slave2_url = "http://" + std::string(argv[4]);


    memc = memcached_create(NULL);
    memcached_server_add(memc, "127.0.0.1", 11211);

  
    std::thread mqtt_thread(mqtt_worker);

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    std::string listen_url = "http://0.0.0.0:" + port;
    mg_http_listen(&mgr, listen_url.c_str(), fn, NULL);
    
    std::cout << "Master Node on " << listen_url << " (HTTP & MQTT enabled)\n";
    for (;;) mg_mgr_poll(&mgr, 1000);
    
    mqtt_thread.join();
    memcached_free(memc);
    mg_mgr_free(&mgr);
    return 0;
}
