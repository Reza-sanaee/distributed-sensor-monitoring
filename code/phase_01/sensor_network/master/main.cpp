#include <iostream>
#include <string>
#include <cstring>
#include <sqlite3.h>
#include <cstdio>
#include "mongoose.h"


struct AppConfig {
    std::string db_path;
    std::string slave1_url;
    std::string slave2_url;
};

AppConfig config;


std::string query_local_db(const std::string& type, const std::string& id) {
    sqlite3* db;
    if (sqlite3_open(config.db_path.c_str(), &db) != SQLITE_OK) {
        return "{\"error\": \"Database connection failed\"}";
    }

    std::string query = "SELECT r.value, s.unit FROM sensors s "
                        "JOIN sensor_readings r ON s.sensor_id = r.sensor_id "
                        "WHERE s.sensor_type = ? AND s.sensor_id = ? "
                        "ORDER BY r.recorded_at DESC LIMIT 1;";

    sqlite3_stmt* stmt;
    std::string result_json = ""; 

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string unit = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            result_json = "{\"status\": \"success\", \"value\": " + val + ", \"unit\": \"" + unit + "\", \"node\": \"Master\"}";
        }
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result_json;
}


std::string fetch_from_slave(const std::string& base_url, const std::string& type, const std::string& id) {
    std::string result = "";
    char buffer[128];

    std::string cmd = "curl -s -m 2 \"" + base_url + "/api/sensor?type=" + type + "&id=" + id + "\"";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}


std::string find_sensor_data(const std::string& type, const std::string& id) {

    std::string local_result = query_local_db(type, id);
    if (!local_result.empty()) {
        return local_result;
    }

    // 2. Query slave 1
    std::cout << "Data not in Master, asking Slave 1..." << std::endl;
    std::string s1_result = fetch_from_slave(config.slave1_url, type, id);
    if (s1_result.find("\"status\": \"success\"") != std::string::npos) {
        // Add the node name to identify the response source
        s1_result.insert(s1_result.length() - 1, ", \"node\": \"Slave1\"");
        return s1_result;
    }

    // 3. Query slave 2
    std::cout << "Data not in Slave 1, asking Slave 2..." << std::endl;
    std::string s2_result = fetch_from_slave(config.slave2_url, type, id);
    if (s2_result.find("\"status\": \"success\"") != std::string::npos) {
        s2_result.insert(s2_result.length() - 1, ", \"node\": \"Slave2\"");
        return s2_result;
    }

    // 4. No record was found on any node
    return "{\"error\": \"Sensor data not found in any node (Master, Slave1, Slave2)\"}";
}

// HTTP request handler (API) for the master
static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        
        if (hm->uri.len >= 11 && strncmp(hm->uri.buf, "/api/sensor", 11) == 0) {
            char type[50] = {0}, id[50] = {0};
            
            mg_http_get_var(&hm->query, "type", type, sizeof(type));
            mg_http_get_var(&hm->query, "id", id, sizeof(id));

            if (strlen(type) > 0 && strlen(id) > 0) {
                std::string json_response = find_sensor_data(type, id);
                if(json_response.find("error") != std::string::npos) {
                    mg_http_reply(c, 404, "Content-Type: application/json\r\n", "%s", json_response.c_str());
                } else {
                    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", json_response.c_str());
                }
            } else {
                mg_http_reply(c, 400, "Content-Type: application/json\r\n", "%s", "{\"error\": \"Missing type or id parameters\"}");
            }
        } else {
            mg_http_reply(c, 404, "Content-Type: application/json\r\n", "%s", "{\"error\": \"Endpoint not found\"}");
        }
    }
}

int main(int argc, char *argv[]) {
    // Master Six arguments provide explicit runtime configuration
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <PORT> <DB_PATH> <SLAVE1_IP:PORT> <SLAVE2_IP:PORT>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 8080 ../data/master.db 192.168.64.129:8081 192.168.64.130:8082" << std::endl;
        return 1;
    }

    std::string port = argv[1];
    config.db_path = argv[2];
    config.slave1_url = "http://" + std::string(argv[3]);
    config.slave2_url = "http://" + std::string(argv[4]);

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    
    std::string listen_url = "http://0.0.0.0:" + port;
    mg_http_listen(&mgr, listen_url.c_str(), fn, NULL);
    
    std::cout << "Master Node starting on " << listen_url << std::endl;
    std::cout << "Local DB: " << config.db_path << std::endl;
    std::cout << "Slave 1: " << config.slave1_url << std::endl;
    std::cout << "Slave 2: " << config.slave2_url << std::endl;

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }
    
    mg_mgr_free(&mgr);
    return 0;
}
