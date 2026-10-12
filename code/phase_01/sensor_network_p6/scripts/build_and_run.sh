#!/bin/bash

check_and_install() {
    PACKAGE=$1
    if ! dpkg -s $PACKAGE >/dev/null 2>&1; then
        echo -e "\e[33mPackage '$PACKAGE' is not installed on your system.\e[0m"
        read -p "Do you want to install it? (y/n): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]
        then
            echo "Installing $PACKAGE..."
            sudo apt-get update && sudo apt-get install -y $PACKAGE
        else
            echo "Warning: Without $PACKAGE, compilation or execution may fail."
        fi
    else
        echo -e "\e[32m✔ Package '$PACKAGE' is already installed.\e[0m"
    fi
}

echo "=== Checking System Requirements (Part 6 - Alert Daemon) ==="
check_and_install "g++"
check_and_install "make"
check_and_install "libsqlite3-dev"
check_and_install "memcached"
check_and_install "libmemcached-dev"
check_and_install "mosquitto"
check_and_install "mosquitto-clients"
check_and_install "libmosquitto-dev"
check_and_install "snmp"
check_and_install "snmpd"
check_and_install "libsnmp-dev"
check_and_install "jq"

echo -e "\n=== Initializing Databases ==="
if [ -f "init_logs_db.sh" ]; then
    chmod +x init_logs_db.sh
    echo "Running init_logs_db.sh to populate historical data..."
    ./init_logs_db.sh
fi

if [ -f "init_alerts.sh" ]; then
    chmod +x init_alerts.sh
    echo "Running init_alerts.sh to initialize alerts table..."
    ./init_alerts.sh
    echo "✔ Alerts database initialized."
else
    echo "Warning: init_alerts.sh not found."
fi

echo -e "\n=== Configuring SNMPd Auto-Pass ==="
if [ -f "snmp_pass.sh" ]; then
    chmod +x snmp_pass.sh
    SCRIPTS_DIR=$(pwd)
    PASS_LINE="pass .1.3.6.1.4.1.55555 /bin/bash $SCRIPTS_DIR/snmp_pass.sh"
    SNMP_CONF="/etc/snmp/snmpd.conf"

    if grep -qF ".1.3.6.1.4.1.55555" "$SNMP_CONF"; then
        echo "✔ SNMP configuration already exists."
    else
        echo "Adding custom OID configuration to SNMPd..."
        echo "$PASS_LINE" | sudo tee -a "$SNMP_CONF" > /dev/null
        sudo systemctl restart snmpd
        echo "✔ SNMPd config updated and service restarted."
    fi
fi

echo -e "\n=== Starting Background Services ==="
sudo systemctl start memcached
sudo systemctl enable memcached
sudo systemctl start mosquitto
sudo systemctl enable mosquitto
sudo systemctl start snmpd
sudo systemctl enable snmpd
echo "Memcached, Mosquitto, and SNMPd services are active."

echo -e "\n=== Nodes Network Configuration ==="
echo "Please enter the connection settings (leave blank to use defaults):"

read -p "Master Node Port [Default 8080]: " MASTER_PORT
MASTER_PORT=${MASTER_PORT:-8080}

read -p "Slave 1 IP and Port [Default 127.0.0.1:8081]: " SLAVE1_IP_PORT
SLAVE1_IP_PORT=${SLAVE1_IP_PORT:-127.0.0.1:8081}

read -p "Slave 2 IP and Port [Default 127.0.0.1:8082]: " SLAVE2_IP_PORT
SLAVE2_IP_PORT=${SLAVE2_IP_PORT:-127.0.0.1:8082}

echo -e "\n=== Compiling Slave Node ==="
if [ -d "../slave" ]; then
    cd ../slave
    make clean
    make
    cd ../scripts
else
    echo "Warning: 'slave' directory not found!"
fi

echo -e "\n=== Compiling Master Node & Alert Daemon ==="
if [ -d "../master" ]; then
    cd ../master
    make clean
    make
    cd ../scripts
else
    echo "Error: 'master' directory not found!"
    exit 1
fi

echo -e "\n=== Installing Alert Daemon Service ==="
if [ -f "install_daemon.sh" ]; then
    chmod +x install_daemon.sh
    echo "Executing install_daemon.sh to setup systemd service..."
    ./install_daemon.sh
else
    echo "Warning: install_daemon.sh not found in scripts folder."
fi

echo -e "\n=== Build Completed Successfully ==="
echo "To run the Slave nodes, open new terminals and use:"
echo "cd ../slave && ./slave_node 8081 ./slave1.db"
echo "cd ../slave && ./slave_node 8082 ./slave2.db"
echo "--------------------------------------------------------"
echo "The command to run the Master Node is:"
echo "cd ../master && ./master_node $MASTER_PORT ./master.db $SLAVE1_IP_PORT $SLAVE2_IP_PORT"
echo "--------------------------------------------------------"
echo "You can check the Alert Daemon status using: sudo systemctl status alert_daemon"

read -p "Do you want to run the Master Node now? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]
then
    echo "Running Master Node..."
    cd ../master
    ./master_node $MASTER_PORT ./master.db $SLAVE1_IP_PORT $SLAVE2_IP_PORT
else
    echo "You can manually run the commands above."
fi
