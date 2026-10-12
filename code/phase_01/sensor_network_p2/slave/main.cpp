#include <iostream>
#include <string>
#include <cstring>
#include <sqlite3.h>
#include <libmemcached/memcached.h>
#include <chrono>
#include "mongoose.h"

struct AppConfig {
    std::string db_path;
};

AppConfig app_config;
memcached_st *memc; 


std::string query_sensor_with_cache(const std::string& type, const std::string& id) {

    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::string cache_key = type + "_" + id;
    memcached_return_t rc;
    size_t value_length;
    uint32_t flags;
    

    char* cache_result = memcached_get(memc, cache_key.c_str(), cache_key.length(), &value_length, &flags, &rc);
    
    if (rc == MEMCACHED_SUCCESS && cache_result != nullptr) {
        
        std::string val(cache_result);
        free(cache_result);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end_time - start_time;
        
        return "{\"status\": \"success\", \"data\": " + val + ", \"source\": \"Memcached\", \"response_time_ms\": " + std::to_string(duration.count()) + "}";
    }
    

    sqlite3* db;
    if (sqlite3_open(app_config.db_path.c_str(), &db) != SQLITE_OK) {
        return "{\"error\": \"Database connection failed\"}";
    }

    std::string query = "SELECT r.value, s.unit FROM sensors s "
                        "JOIN sensor_readings r ON s.sensor_id = r.sensor_id "
                        "WHERE s.sensor_type = ? AND s.sensor_id = ? "
                        "ORDER BY r.recorded_at DESC LIMIT 1;";

    sqlite3_stmt* stmt;
    std::string result_json = "{\"error\": \"Not found\"}";

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string unit = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            
       
            std::string data_to_cache = "{\"value\": " + val + ", \"unit\": \"" + unit + "\"}";
            
      
            memcached_set(memc, cache_key.c_str(), cache_key.length(), data_to_cache.c_str(), data_to_cache.length(), 60, 0);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = end_time - start_time;
            
            result_json = "{\"status\": \"success\", \"data\": " + data_to_cache + ", \"source\": \"SQLite\", \"response_time_ms\": " + std::to_string(duration.count()) + "}";
        }
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result_json;
}

static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        
        if (hm->uri.len >= 11 && strncmp(hm->uri.buf, "/api/sensor", 11) == 0) {
            char type[50] = {0}, id[50] = {0};
            
            mg_http_get_var(&hm->query, "type", type, sizeof(type));
            mg_http_get_var(&hm->query, "id", id, sizeof(id));

            if (strlen(type) > 0 && strlen(id) > 0) {
                std::string json_response = query_sensor_with_cache(type, id);
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
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <PORT> <DB_PATH>" << std::endl;
        return 1;
    }

    std::string port = argv[1];
    app_config.db_path = argv[2];


    memc = memcached_create(NULL);
    memcached_server_add(memc, "127.0.0.1", 11211);

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    
    std::string listen_url = "http://0.0.0.0:" + port;
    mg_http_listen(&mgr, listen_url.c_str(), fn, NULL);
    
    std::cout << "Slave Node (with Memcached) starting on " << listen_url << " using DB: " << app_config.db_path << std::endl;

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }
    
    memcached_free(memc); 
    mg_mgr_free(&mgr);
    return 0;
}
