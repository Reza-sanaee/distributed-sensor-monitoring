# Phase 4 execution guide: Read sensors through SNMP

The master exposes aggregated sensor information (including name, type, and latest value) through SNMP for system monitoring. 

---

## 1. Automatic installation and configuration (Automated Setup)

The integrated script `build_and_run.sh` can install and configure the services. It checks the required:
1. packages `snmp`, `snmpd` and the tool `jq` and installs them.
2. the line for `pass` custom (OID `.1.3.6.1.4.1.55555`) is added dynamically to `/etc/snmp/snmpd.conf` the configuration.
3. service `snmpd` is restarted and enabled in the background.

Run the script to perform these steps:
```bash
cd scripts
./build_and_run.sh
```

## 2. Directory permissions (Required for execution)
The Linux snmpd service runs under a restricted account (the snmp user) runs. For it to locate and execute snmp_pass.sh grant directory traversal permission (Execute) along the project path.

Use the following commands (Replace reza if your Linux account has another name):

```bash
chmod o+x /home/reza
chmod o+x /home/reza/sensor_network_p4
chmod o+x /home/reza/sensor_network_p4/scripts
```

## 3. Start the system
This phase depends on the API from earlier phases (including distributed retrieval and caching) for operation. Ensure the network is running before testing:

Start the slaves: Run slaves 1 and 2 in separate terminals.

Start the master: At the end of build_and_run.sh, enter y to start the configured master.


## 4. Read the data (Testing)

Use the standard SNMP tool to read current values `snmpwalk` from the command line. 

In a new terminal (with all network nodes running) enter the following command:

```bash
snmpwalk -v2c -c public localhost .1.3.6.1.4.1.55555
```

Expected output:
snmpd and the pass script forward the request to the master API. The terminal prints an OID tree with each sensor's name and type (such as temperature, CO2, or smoke) and the latest stored value across the three nodes.
