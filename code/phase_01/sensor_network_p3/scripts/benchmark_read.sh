#!/bin/bash

MASTER_URL="http://127.0.0.1:8080"

echo "============================================="
echo "   BENCHMARK SCRIPT - ROUND 1 (CACHE MISS)   "
echo "============================================="
echo "Reading from Master Local DB:"
curl -s "$MASTER_URL/api/sensor?type=temperature&id=101" | jq .
echo -e "\nReading from Slave 1 (via Master):"
curl -s "$MASTER_URL/api/sensor?type=co2&id=204" | jq .
echo -e "\nReading from Slave 2 (via Master):"
curl -s "$MASTER_URL/api/sensor?type=smoke&id=304" | jq .

echo -e "\n============================================="
echo "   BENCHMARK SCRIPT - ROUND 2 (CACHE HIT)    "
echo "============================================="
echo "Reading from Master Local DB:"
curl -s "$MASTER_URL/api/sensor?type=temperature&id=101" | jq .
echo -e "\nReading from Slave 1 (via Master):"
curl -s "$MASTER_URL/api/sensor?type=co2&id=204" | jq .
echo -e "\nReading from Slave 2 (via Master):"
curl -s "$MASTER_URL/api/sensor?type=smoke&id=304" | jq .
echo ""
