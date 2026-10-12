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

echo "=== Checking System Requirements ==="
check_and_install "g++"
check_and_install "make"
check_and_install "libsqlite3-dev"

echo -e "\n=== Nodes Network Configuration ==="
echo "Please enter the connection settings (leave blank to use defaults):"

read -p "Master Node Port [Default 8080]: " MASTER_PORT
MASTER_PORT=${MASTER_PORT:-8080}

read -p "Slave 1 IP and Port (e.g., 192.168.1.10:8081) [Default 127.0.0.1:8081]: " SLAVE1_IP_PORT
SLAVE1_IP_PORT=${SLAVE1_IP_PORT:-127.0.0.1:8081}

read -p "Slave 2 IP and Port (e.g., 192.168.1.11:8082) [Default 127.0.0.1:8082]: " SLAVE2_IP_PORT
SLAVE2_IP_PORT=${SLAVE2_IP_PORT:-127.0.0.1:8082}

echo -e "\n=== Compiling Master Node ==="
cd ../master || { echo "Error: 'master' directory not found!"; exit 1; }
make clean
make

echo -e "\n=== Build Completed Successfully ==="
echo "The final command to run the system is:"
echo "./master_node $MASTER_PORT ./master.db $SLAVE1_IP_PORT $SLAVE2_IP_PORT"
echo "--------------------------------------------------------"

read -p "Do you want to run the Master Node now? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]
then
    echo "Running Master Node..."
    ./master_node $MASTER_PORT ./master.db $SLAVE1_IP_PORT $SLAVE2_IP_PORT
else
    echo "You can manually run the command above in the 'master' directory."
fi
