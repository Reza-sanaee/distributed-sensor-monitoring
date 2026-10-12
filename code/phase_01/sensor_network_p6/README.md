# Phase 6 documentation and execution: Alert system (Alert Daemon)

This phase implements a daemon (Daemon) for Linux in C++ that runs independently in the background and periodically checks sensor values to generate and store alerts.

---

## Part 1: Build, install, and start (Automatic execution)

Database setup (`init_alerts.sh`)program compilation (`Makefile`) and background-service installation (`install_daemon.sh`) are integrated into the main project script. 

Run the following script to deploy the alert daemon and the nodes:
```bash
cd scripts
chmod +x build_and_run.sh
./build_and_run.sh
```

It creates the sensor-alert service dynamically (without a fixed project path) and starts it in the Linux background.

## Part 2: Alert-system management and monitoring
After the installation script finishes, the alert service is running. Use the following commands to manage and test it:

## 1. Service status (Systemd)
Inspect status (Status): to check service health and activation:

```bash
sudo systemctl status sensor-alert
```

Stop monitoring (Stop):

```bash
sudo systemctl stop sensor-alert
```

Start again (Start):

```bash
sudo systemctl start sensor-alert
```

## 2. System logs (Live Monitoring)
Live program output and logs (including generated alerts) are available through Linux logging and journalctl:

```bash
sudo journalctl -u sensor-alert -f
```

## 3. Inspect database alerts
The daemon stores alerts persistently in the master database. Use the following command to inspect them:

```bash
sqlite3 master/master.db "SELECT * FROM alerts;"
```

## Part 3: Architecture and system logic


## 1. Operation diagram (Data Flow)
alert_daemon uses a five-second polling loop (sleep(5)) for repeated monitoring:

Retrieve records: In every cycle, the latest stored record (MAX(recorded_at)) for each sensor is read from sensor_readings.

Validation: Values are checked against four alert conditions.

Duplicate suppression (Spam Prevention): Before insertion, check for a matching alert with NEW status (unreviewed) for the same sensor. An existing matching alert suppresses another identical record.

Record the event: Insert a new NEW alert into alerts and print the corresponding log message.


## 2. Alert-table schema (Database Schema)
The alerts table stores alert history with the following fields:

```sql
CREATE TABLE alerts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sensor_id TEXT,
    sensor_name TEXT,
    alert_type TEXT,
    sensor_value TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    status TEXT
);
```

## 3. Alert conditions
A violated condition produces its associated alert type:

High temperature (HIGH_TEMPERATURE): Temperature above 40 degrees Celsius.

Humidity outside the permitted range (HUMIDITY_OUT_OF_BOUNDS): Humidity below 20 percent or above 80 percent.

Invalid readings (INVALID_DATA): Values outside the configured validity range (from -50 to 1000) for any sensor; the condition flags a potentially invalid reading.

Stale sensor (TIMEOUT_NO_DATA): Current time minus the sensor's latest recorded_at exceeds 300 seconds (five minutes) and produces a no-new-data alert.
