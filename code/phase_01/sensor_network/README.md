## Hotel Sensor Monitoring System (Distributed Database)
This project implements the first stage of a hotel sensor-monitoring system. The architecture uses separate databases on one master and two slave nodes. The nodes are implemented in C++ and use Mongoose for network communication (HTTP Server/Client) and SQLite for storage.

## Project structure
```text
sensor_network/
├── master/
│   ├── main.cpp         # Master-node source (routing and HTTP server)
│   ├── mongoose.c       # Mongoose library
│   ├── mongoose.h
│   ├── master_init_db.sh
│   └── Makefile         # Master build file
│
├── slave/
│   ├── main.cpp         # Slave-node source (query responses)
│   ├── mongoose.c       
│   ├── mongoose.h
│   ├── slave2_init_db.sh 
│   ├── slave1_init_db.sh
│   └── Makefile         # Slave build file
│
├── scripts/
│   └── build_and_run.sh # Interactive build and execution script
├── data/                # Initial CSV datasets
├── report.md            # Complete system architecture and design report
└── README.md            # Project execution guide
```

## Prerequisites and dependency installation
The `build_and_run.sh` script detects missing dependencies and asks whether to install them. For manual installation on Ubuntu 22.04, use the following command:

```bash
sudo apt update
sudo apt install -y build-essential libsqlite3-dev sqlite3 curl
```

## Database initialization
Create a SQLite database for each node before starting the servers. Enter scripts and run the following initialization scripts (Ensure that the CSV data files are in ../data/ the specified location):

```bash
cd master
chmod +x master_init_db.sh
./master_init_db.sh master.db ../data/master_sensors.csv
cd ../slave
chmod +x slave1_init_db.sh slave2_init_db.sh
./slave1_init_db.sh slave1.db ../data/slave1_sensors.csv
./slave2_init_db.sh slave2.db ../data/slave2_sensors.csv
```

Copy the generated .db files to the corresponding virtual machines or execution directories.

## Build and network configuration 
The interactive `build_and_run.sh` scriptavoids hard-coded addresses and long manual command lines. It obtains configuration through interactive command-line prompts.

## 1. Start the slave nodes (in separate virtual machines or terminals)
Build and start the slaves first so they are ready to answer requests.

For slave 1:

```bash
cd slave
make
./slave_node 8081 /path/to/slave1.db
```

For slave 2:

```bash
cd slave
make
./slave_node 8082 /path/to/slave2.db
```

## 2. Build, configure, and start the master
Enter scripts and run the build script:

```bash
cd scripts
./build_and_run.sh
```

The script performs the following steps in order:

Dependency checks: If g++ or libsqlite3-dev is missing, an installation prompt appears.

Master port: Enter the port on which the master will listen (default: 8080).

Slave configuration: Enter the IP addresses and ports of slaves 1 and 2 for routing (Defaults are 127.0.0.1:8081 and 127.0.0.1:8082 respectively).

Automatic execution: After building, the script asks whether to start the master immediately. Enter y to start it with the supplied settings.

## Request-path testing 

Send operator requests only to the master. The master checks its local database, then queries slave 1 and slave 2 in sequence if the record is absent.

(Note: The examples below use 127.0.0.1 and port 8080. Replace these values if your configured addresses differ).

Test local-database retrieval (Master):

```bash
curl -s "http://127.0.0.1:8080/api/sensor?type=temperature&id=101"
```

Test retrieval through slave 1:

```bash
curl -s "http://127.0.0.1:8080/api/sensor?type=co2&id=204"
```

Test retrieval through slave 2:

```bash
curl -s "http://127.0.0.1:8080/api/sensor?type=smoke&id=304"
```

Example successful response (in JSON):

```json
{"status": "success", "data": {"value": 735, "unit": "ppm"}, "source": "Network", "node": "Slave1", "response_time_ms": 12.5}
```

