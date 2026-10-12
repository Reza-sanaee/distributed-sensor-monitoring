#!/bin/bash


MASTER_DB="/home/reza/sensor_network_p5/master/master.db"


sqlite3 $MASTER_DB "INSERT INTO sensor_readings (sensor_id, value, recorded_at) VALUES ('101', 24.8, '2026-06-01 10:15:00');"
sqlite3 $MASTER_DB "INSERT INTO sensor_readings (sensor_id, value, recorded_at) VALUES ('101', 25.1, '2026-06-01 10:30:00');"
sqlite3 $MASTER_DB "INSERT INTO sensor_readings (sensor_id, value, recorded_at) VALUES ('101', 25.4, '2026-06-01 10:45:00');"

echo "Master Database initialized with historical logs successfully."
