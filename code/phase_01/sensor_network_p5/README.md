# Phase 5 execution and architecture: Sensor-history API (History API)

This phase adds a Mongoose API for sensor history (Logs) on a specified date. The setup procedure and architecture are described below.

---

## Part 1: API architecture and design

The request and data-flow structure follows the project requirements.

### 1. API design
The endpoint uses a REST-style interface. The master acts as the retrieval gateway. It receives a request and first queries its local database. If the sensor is absent locally, the request is forwarded sequentially to the slaves and the result is returned to the client.

### 2. Request structure (Request Structure)
Requests use method `GET` with three query-string parameters.
*   **Path (Endpoint):** `/api/sensor/logs`
*   **Input parameters:**
    *   `sensor_name`: Sensor name (for example temperature)
    *   `sensor_id`: Unique sensor identifier (for example 101)
    *   `date`: Date for history retrieval (YYYY-MM-DD format)

**Example request:**
```text
GET /api/sensor/logs?sensor_name=temperature&sensor_id=101&date=2026-06-01
```

### 3. Response structure (Response Structure)
The system returns JSON. with sensor metadata and an array of values and timestamps for the requested date. A suitable error response is returned when no records exist for that date.

Example successful response (JSON):

```json
{
  "sensor_name": "temperature",
  "sensor_id": "101",
  "date": "2026-06-01",
  "values": [
    {
      "time": "10:15:00",
      "value": "24.8"
    },
    {
      "time": "10:30:00",
      "value": "25.1"
    }
```

  ]
}
### 4. Database retrieval path (Data Flow)
1. Request reception: The Mongoose server receives HTTP and extracts the parameters.
2. Local lookup (SQLite): query_local_logs executes SELECT on sensor_readings in master.db with sensor-name, identifier, and date filters.
3. Network routing (if required): If the local result is empty, the master sends the same request to slave 1 using curl. and tries slave 2 if no response is obtained.
4. Return the result: The retrieved records are encoded as JSON and sent through mg_http_reply to the client (user) as the response.



### Part 2: System startup and testing

### 1. Initialize the databases and build

The integrated script initializes databases and builds the programs with their Makefiles.

Enter scripts and run the following script:

```bash
cd scripts
chmod +x build_and_run.sh
./build_and_run.sh
```

Note: The init_logs_db script.sh runs during setup to insert test records (Mock Data) for specified dates.


### 2. Start the network nodes

after a successful build:
1. Start the slaves in separate terminals:

```bash
cd slave
./slave_node 8081 ../master/slave1.db
./slave_node 8082 ../master/slave2.db
```

2. At the end of build_and_run.shconfirm master startup on port 8080.

3. Test history retrieval
In a new terminal, use the following API tests (jq can format the returned JSON):

Test 1: Local-database query (Master)

```bash
curl -s "http://127.0.0.1:8080/api/sensor/logs?sensor_name=temperature&sensor_id=101&date=2026-06-01" | jq
```

Test 2: Route to slave 1 (CO2 gas sensor)

```bash
curl -s "http://127.0.0.1:8080/api/sensor/logs?sensor_name=co2&sensor_id=204&date=2026-06-01" | jq
```

Test 3: Error response (Date without records)

```bash
curl -s "http://127.0.0.1:8080/api/sensor/logs?sensor_name=temperature&sensor_id=101&date=2026-06-02" | jq
```

Expected output:

```json
{
  "error": "No records found for the requested date"
}
```

