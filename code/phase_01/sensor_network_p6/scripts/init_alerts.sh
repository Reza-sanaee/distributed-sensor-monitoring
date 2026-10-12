#!/bin/bash

# Master database path
DB_PATH="../master/master.db"

# 1. Enable concurrent reads/and writes (for database access by multiple processes)
sqlite3 $DB_PATH "PRAGMA journal_mode=WAL;"

# 2. Create the specified alert-table schema.png
sqlite3 $DB_PATH "CREATE TABLE IF NOT EXISTS alerts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sensor_id TEXT,
    sensor_name TEXT,
    alert_type TEXT,
    sensor_value TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    status TEXT
);"

# 3. Insert test data (high temperature and stale timestamps) to exercise the daemon
sqlite3 $DB_PATH "INSERT INTO sensor_readings (sensor_id, value, recorded_at) VALUES ('101', 99.9, datetime('now'));"
sqlite3 $DB_PATH "INSERT INTO sensor_readings (sensor_id, value, recorded_at) VALUES ('102', 25.0, datetime('now', '-10 minutes'));"

echo "Alerts table created successfully. WAL mode enabled."
