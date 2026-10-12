#!/bin/bash


BASE=".1.3.6.1.4.1.55555"
CMD=$1
REQ_OID=$2


API_URL="http://127.0.0.1:8080"


# .1 = Sensor 101 (Temp), .2 = Sensor 204 (CO2), .3 = Sensor 304 (Smoke)
OIDS=(
    "$BASE.1.1|string|Master Temperature Sensor"
    "$BASE.1.2|string|temperature"
    "$BASE.1.3|integer|API:temperature:101"
    
    "$BASE.2.1|string|Slave 1 CO2 Sensor"
    "$BASE.2.2|string|co2"
    "$BASE.2.3|integer|API:co2:204"
    
    "$BASE.3.1|string|Slave 2 Smoke Sensor"
    "$BASE.3.2|string|smoke"
    "$BASE.3.3|integer|API:smoke:304"
)


fetch_value() {
    local source=$1
    if [[ "$source" == API:* ]]; then
        IFS=':' read -r prefix type id <<< "$source"
       
        val=$(curl -s -m 2 "$API_URL/api/sensor?type=$type&id=$id" | jq -r '.data.value // empty')
        
       
        if [ -z "$val" ] || [ "$val" == "null" ]; then
            echo "0"
        else
        
            printf "%.0f\n" "$val"
        fi
    else
        echo "$source"
    fi
}

get_oid() {
    for entry in "${OIDS[@]}"; do
        IFS='|' read -r oid type source <<< "$entry"
        if [ "$oid" == "$1" ]; then
            val=$(fetch_value "$source")
            echo "$oid"
            echo "$type"
            echo "$val"
            exit 0
        fi
    done
    exit 0
}

get_next_oid() {
    for entry in "${OIDS[@]}"; do
        IFS='|' read -r oid type source <<< "$entry"
      
        if [ "$(printf '%s\n' "$1" "$oid" | sort -V | head -n1)" == "$1" ] && [ "$1" != "$oid" ]; then
            val=$(fetch_value "$source")
            echo "$oid"
            echo "$type"
            echo "$val"
            exit 0
        fi
    done
    exit 0
}


if [ "$CMD" == "-g" ]; then
    get_oid "$REQ_OID"
elif [ "$CMD" == "-n" ]; then
    if [ "$REQ_OID" == "$BASE" ]; then
         IFS='|' read -r oid type source <<< "${OIDS[0]}"
         val=$(fetch_value "$source")
         echo "$oid"
         echo "$type"
         echo "$val"
         exit 0
    fi
    get_next_oid "$REQ_OID"
fi
exit 0
