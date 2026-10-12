#include <iostream>
#include <sqlite3.h>
#include <unistd.h>
#include <string>

using namespace std;

string DB_PATH = "master.db";

// Insert an alert into the database
void insert_alert(sqlite3* db, const string& sensor_id, const string& sensor_name, const string& alert_type, const string& value) {
    string check_query = "SELECT count(*) FROM alerts WHERE sensor_id='" + sensor_id + "' AND alert_type='" + alert_type + "' AND status='NEW';";
    sqlite3_stmt* stmt;
    int count = 0;
    if (sqlite3_prepare_v2(db, check_query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (count > 0) return; 

    string query = "INSERT INTO alerts (sensor_id, sensor_name, alert_type, sensor_value, status) VALUES ('" +
                   sensor_id + "', '" + sensor_name + "', '" + alert_type + "', '" + value + "', 'NEW');";
    char* err_msg = 0;
    if (sqlite3_exec(db, query.c_str(), 0, 0, &err_msg) != SQLITE_OK) {
        cout << "[ERROR] DB Write Failed: " << err_msg << endl;
        sqlite3_free(err_msg);
    } else {
        cout << "[ALERT LOGGED] Sensor " << sensor_id << " (" << sensor_name << ") | " << alert_type << " | Value: " << value << endl;
    }
}

// Monitor sensors
void monitor_sensors() {
    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) return;
    sqlite3_busy_timeout(db, 3000); 

    // Independent sensor query (without requiring a sensors table)
    string query = "SELECT sensor_id, value, "
                   "(strftime('%s', 'now') - strftime('%s', recorded_at)) as seconds_ago "
                   "FROM sensor_readings "
                   "WHERE recorded_at = (SELECT MAX(recorded_at) FROM sensor_readings r2 WHERE r2.sensor_id = sensor_readings.sensor_id);";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            string id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            double value = sqlite3_column_double(stmt, 1);
            int seconds_ago = sqlite3_column_int(stmt, 2);
            string val_str = to_string(value);

            // Determine sensor type from its identifier
            string type = "Unknown";
            if (id == "101") type = "temperature";
            else if (id == "102" || id == "103") type = "humidity";
            else if (id == "204") type = "co2";
            else type = "Sensor_" + id;

            // Evaluate alert conditions
            if (seconds_ago > 300) {
                insert_alert(db, id, type, "TIMEOUT_NO_DATA", "Last seen " + to_string(seconds_ago) + "s ago");
            }
            if (value < -50 || value > 1000) {
                insert_alert(db, id, type, "INVALID_DATA", val_str);
            }
            if (type == "temperature" && value > 40.0) {
                insert_alert(db, id, type, "HIGH_TEMPERATURE", val_str);
            }
            if (type == "humidity" && (value < 20.0 || value > 80.0)) {
                insert_alert(db, id, type, "HUMIDITY_OUT_OF_BOUNDS", val_str);
            }
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


int main(int argc, char* argv[]) {
   
    if (argc > 1) {
        DB_PATH = argv[1];
    }
    cout << "IoT Alert Daemon Started Successfully." << endl;
    while (true) {
        monitor_sensors();
        sleep(5); 
    }
    return 0;
}
