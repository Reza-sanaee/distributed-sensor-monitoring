#!/bin/bash


SCRIPTS_DIR=$(pwd)
cd ../master
MASTER_DIR=$(pwd)
DAEMON_EXEC="$MASTER_DIR/alert_daemon"
DB_FILE="$MASTER_DIR/master.db"

SERVICE_FILE="/etc/systemd/system/sensor-alert.service"

echo "Installing Alert Daemon Service..."
echo "Executable Path: $DAEMON_EXEC"
echo "Database Path: $DB_FILE"


sudo bash -c "cat > $SERVICE_FILE" << EOL
[Unit]
Description=IoT Sensor Alert Daemon
After=network.target

[Service]
Type=simple
WorkingDirectory=$MASTER_DIR

ExecStart=$DAEMON_EXEC $DB_FILE
Restart=always
RestartSec=3
User=root

[Install]
WantedBy=multi-user.target
EOL


sudo systemctl daemon-reload
sudo systemctl enable sensor-alert
sudo systemctl restart sensor-alert

echo "✔ Service 'sensor-alert' installed and started successfully!"
