# Distributed Sensor Monitoring with HTTP, MQTT, SNMP, and SQLite

A six-stage C++ distributed monitoring prototype uses a master and two slave Linux nodes with separate SQLite stores. It adds cache-aside Memcached access, MQTT request/reply, an SNMP bridge, historical queries, and a systemd alert daemon. Its strongest contribution is integration of multiple systems interfaces, with measured cache effects and clearly bounded monitoring coverage.

## Technical highlights

- Built a three-node C++ sensor-monitoring prototype with Mongoose HTTP APIs, SQLite, Memcached, MQTT, and an SNMP bridge.
- Implemented local/remote retrieval, date-based history queries, and a systemd alert daemon; compared cache-miss and cache-hit response times.

## Explore the project

- [Source and execution guide](code/RUNNING.md)
- [Technical report (PDF)](report/Report_EN.pdf)
- [Technical report (Markdown)](report/Report_EN.md)
- [Assignment/specification (PDF)](assignment/Assignment_EN.pdf)

## Reproduction and attribution

Read the execution guide and report before running the code. Dependencies, input data, licensed tools, and hardware requirements vary by project. Recorded results are distinguished from rerun results in the accompanying documentation. Team and third-party source attribution in the reports and source headers is retained. This portfolio does not claim independent authorship of supplied frameworks or dependencies.
