# Phase 3 execution guide: MQTT request and response

The master gains an MQTT message interface. It retains the two-layer cache (Memcached + SQLite) from the preceding phase and accepts sensor requests over HTTP or publish/Subscribe and responds through messaging.

---

## 1. Broker and dependency installation

The integrated interactive script `build_and_run.sh` checks for the broker `Mosquitto` and its development libraries and can install them. The service `mosquitto` starts in the background after installation (on port 1883) without a separate manual startup step.

For manual setup, run the following Linux command:
```bash
sudo apt update
sudo apt install -y mosquitto mosquitto-clients libmosquitto-dev jq
```

## 2. Build and start the system (Interactive procedure)
The phase Makefiles include MQTT support (with -lmosquitto and -pthread). The automation script obtains addresses interactively:

Step 1: Start the slave nodes
Start the slaves in separate terminals before making requests:

```bash
cd slave
./slave_node 8081 ../master/slave1.db
./slave_node 8082 ../master/slave2.db
```

Step 2: Build and start the master
Enter scripts and run the startup script:

```bash
cd scripts
./build_and_run.sh
```

The script builds the programs and prompts for ports and IP addresses. After final confirmation, a successfully connected master prints [MQTT] Connected to Broker successfully in the terminal log.

## 3. Topic design
MQTT requests and responses use the following topic structure:

Request topic (Publish Request): hotel/sensors/req/<sensor_type>/<sensor_id>

Response topic (Subscribe Response): hotel/sensors/res/<sensor_type>/<sensor_id>

## 4. Manual testing with CLI tools
Use the Mosquitto command-line tools to test request and response behavior:

Step 1: Listen for a response (Subscribe)
In a new terminal, subscribe to the sensor-response topic (for example CO2 sensor 204) and wait for messages:

```bash
mosquitto_sub -h 127.0.0.1 -t "hotel/sensors/res/co2/204" -v
```

(Keep this terminal open to display responses).

Step 2: Send the request (Publish)
In another terminal, publish an empty message to the matching request topic:

```bash
mosquitto_pub -h 127.0.0.1 -t "hotel/sensors/req/co2/204" -m ""
```

The response appears as structured JSON (containing the sensor value, processing time, and data source) in the first terminal.

## 5. Run the request-response benchmark (Benchmark)
The MQTT benchmark compares retrieval latency (for a cache miss requiring disk access/or network access and a cache hit served from RAM) using the supplied benchmark script.

Run the following command on the master machine:

```bash
cd scripts
chmod +x mqtt_benchmark.sh
./mqtt_benchmark.sh
```

The script sends MQTT requests and measures end-to-end elapsed time (Round-trip Response Time) in two consecutive rounds. The observations illustrate cache effects in this IoT prototype (IoT) within the recorded experiment.
