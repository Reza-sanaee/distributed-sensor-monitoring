# SQLite Initialization Files

This package contains SQLite initialization files for a system with one master and two slaves.

## Files supplied to the student

```text
for_students/
├── scripts/
│   ├── master_init_db.sh
│   ├── slave1_init_db.sh
│   ├── slave2_init_db.sh
│   └── read_latest_sample.sh
└── data/
    ├── master_sensors.csv
    ├── slave1_sensors.csv
    └── slave2_sensors.csv
```

## CSV format

```csv
sensor_id,sensor_type,sensor_name,location,value,unit,recorded_at
```

## Execution procedure

```bash
sudo apt update
sudo apt install -y sqlite3

cd for_students/scripts
chmod +x *.sh

./master_init_db.sh master.db ../data/master_sensors.csv
./slave1_init_db.sh slave1.db ../data/slave1_sensors.csv
./slave2_init_db.sh slave2.db ../data/slave2_sensors.csv
```

## Example test

```bash
./read_latest_sample.sh master.db temperature 101
./read_latest_sample.sh slave1.db co2 204
./read_latest_sample.sh slave2.db smoke 304
```

## Private test data

The following directory is for the instructor/or exercise designer and is not part of the student input:

```text
private_test_data_do_not_share/
```

These records use different identifiers and values to reveal hard-coded responses.
