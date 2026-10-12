# Phase 2 execution guide: Memcached caching (Two-layer storage)

This guide documents the second project phase. A cache layer reduces repeated sensor-read latency (Cache) using `Memcached` on both master and slave nodes to form two-layer storage (Memcached + SQLite) for sensor retrieval.

---

## 1. Install Memcached and dependencies

The interactive script can install the required dependencies. It checks which dependencies are missing and requests installation confirmation. For manual installation (including the C development libraries/C++ and the tool `jq`) run the following terminal command:

```bash
sudo apt update
sudo apt install -y memcached libmemcached-dev jq
```

## 2. Start Memcached
The automation script starts memcached (Start and enables it) to accept local connections on port 11211. Check whether the daemon is active (Daemon) with the following command:

```bash
sudo systemctl status memcached
```

## 3. Build the programs
The phase Makefiles link -lmemcached for the cache functions. Run the startup script to build both master and slave:

```bash
cd scripts
./build_and_run.sh
```

The script enters each node directory and cleans (make clean) then rebuilds (make) both programs.

## 4. Start the slaves and master
The integrated startup procedure obtains configuration interactively:

Start the slave nodes: Start the slaves in separate terminals so they can answer requests:

```bash
cd slave
./slave_node 8081 ../master/slave1.db
./slave_node 8082 ../master/slave2.db
```

Start the master: While running build_and_run.sh, enter the master port and slave IP addresses/and ports at the prompts. Enter y at the end to start the master with that configuration.

## 5. Send a request
Send requests only to the master (for example on port 8080) using its configured address. Use curl to obtain a specified sensor reading:

```bash
curl -s "http://127.0.0.1:8080/api/sensor?type=co2&id=204"
```

Example successful response (when the record is read from cache):

```json
{
  "status": "success",
  "data": {
    "value": 735,
    "unit": "ppm"
  },
  "source": "Master_Memcached",
  "node": "Master",
  "response_time_ms": 0.200453
}
```

## 6. Run the latency benchmark (Benchmark)
A Bash benchmark reads all sensors in two consecutive rounds (Round) to compare first and repeated retrieval times. Run it with:

```bash
cd scripts
chmod +x benchmark_read.sh
./benchmark_read.sh
```

## 7. Interpret the benchmark (Cache Miss vs Cache Hit)
The implemented cache uses a cache-aside retrieval path. The recorded benchmark observations are as follows:

First round (Cache Miss): The requested records are initially absent from cache. The system reads them from the main database (SQLite) or queries the slave nodes over the network. Local SQLite reads on the master took approximately 2..4 milliseconds, while network reads (Slavenodes) took approximately 10 to 21 milliseconds.

Second round (Cache Hit): Previously requested records are now in RAM (through Memcached) and can be returned before another database or network lookup. Response times decreased to approximately 0..08 to 0..2 milliseconds.

## Architectural interpretation:
 The recorded pairs show approximately 13- to 100-fold reductions in response time. Caching remote responses at the master reduces network traffic (Network Overhead) for repeated requests during the cache lifetime; these tests do not establish general scalability or reliability.
