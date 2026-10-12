#!/bin/bash

# MQTT broker address (Use localhost because the broker runs on this machine)
BROKER="127.0.0.1"

measure_mqtt() {
    local TYPE=$1
    local ID=$2
    local REQ_TOPIC="hotel/sensors/req/$TYPE/$ID"
    local RES_TOPIC="hotel/sensors/res/$TYPE/$ID"

    # Prepare the background subscriber (Receive one message and exit)
    mosquitto_sub -h $BROKER -t $RES_TOPIC -q 1 -C 1 > /tmp/mqtt_res.json &
    SUB_PID=$!

    # Wait briefly for the subscriber to connect to the broker
    sleep 0.2

    # Record the start time (in nanoseconds)
    START=$(date +%s%N)
    
    # Publish the sensor-read request with QoS 1
    mosquitto_pub -h $BROKER -t $REQ_TOPIC -m "" -q 1

    # Wait for the response and subscriber exit
    wait $SUB_PID
    
    # Record the end time and compute elapsed time (Convert nanoseconds to milliseconds)
    END=$(date +%s%N)
    DIFF_MS=$(( ($END - $START) / 1000000 ))

    echo -e "Response Time (MQTT): \033[1;32m${DIFF_MS} ms\033[0m"
    cat /tmp/mqtt_res.json | jq .
    echo "----------------------------------------"
}

echo "============================================="
echo " MQTT BENCHMARK SCRIPT - ROUND 1 (CACHE MISS)"
echo "============================================="
echo "Reading from Master Local DB:"
measure_mqtt "temperature" "101"

echo "Reading from Slave 1 (via Master):"
measure_mqtt "co2" "204"

echo "Reading from Slave 2 (via Master):"
measure_mqtt "smoke" "304"

echo "============================================="
echo " MQTT BENCHMARK SCRIPT - ROUND 2 (CACHE HIT) "
echo "============================================="
echo "Reading from Master Local DB:"
measure_mqtt "temperature" "101"

echo "Reading from Slave 1 (via Master):"
measure_mqtt "co2" "204"

echo "Reading from Slave 2 (via Master):"
measure_mqtt "smoke" "304"
