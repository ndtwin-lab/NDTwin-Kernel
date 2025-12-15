# NDTwin Changelog

All notable changes to this project (NDTwin source code, Ryu controller program, ...) are documented in this file.

Refer to the git log for more details if you wish.

---

## tag v3.1.0
Tagger: nslab RA <nslab@citi.edu.tw>
Date:   Wed Jun 25 17:33:53 2025 +0800

Major update:
- Enhanced get_graph_data with device_name, mac, and IP array
- Fixed Mininet link bandwidth field bug
- Added APIs: inform_switch_entered, modify_device_name
- Fixed flow handling (add-delete-add crash)
- Improved switch status detection via ping
- Updated host<->switch link stats (bandwidth, flow set)
- Prevented path selection with disabled switches
---

## tag v3.2.0
Tagger: nslab RA <nslab@citi.edu.tw>
Date:   Fri Jul 4 09:32:01 2025 +0800

feat: major NDT enhancements

- Use StaticNetworkTopology.json to describe the complete network topology.
  (get_graph_data API now retrieves the full topology instead of detected topology)
- Refactored ControllerAndOtherEventHandler to use async I/O.
- Store link bandwidth usage every 5 minutes for model training.
- Add device_layer and brand_name metadata at each node. (get_graph_data API can retrieve now)
- Change start_time and end_time to first_sampled_time and latest_sampled_time. (get_graph_data API)
---

## tag v4.0.0
Tagger: nslab RA <nslab@citi.edu.tw>
Date:   Tue Jul 15 12:10:51 2025 +0800

Release v4.0.0: major API and functionality updates

- Changed to preinstalled all-destination routing entries for scalability (no packet-in per flow; removed initial routing policy selection)
- Updated disable_switch to recalculate all-destination routes and return differences (see ndt_api.md)
- Renamed APIs:
    - get_openflow_flow_table -> get_switch_openflow_table_entries
    - get_flow_table_data -> get_detected_flow_data
- Added ability for modify_device_name results to be written to StaticNetworkTopology.json
- Improved get_detected_flow_data to return correct flow 5-tuple information
- Updated get_graph_data API:
    - Flows in flow_set now inserted when detected via sFlow
    - Flows removed if undetected for more than 15 seconds
---

## tag v4.0.1
Tagger: nslab RA <nslab@citi.edu.tw>
Date:   Tue Jul 22 15:55:52 2025 +0800

v4.0.1 major changes:

- Add SimulationRequestManager Module to relay messages between application and simulation server
- Add ApplicationManager Module to handle application registration and setup NFS
- Fix Bugs (like weird doubling topology after 5000s)
- Move HttpSessions function from .hpp to .cpp
- API changes: add /app_register (see ndt_api.md)
- Optimize mutex locks and request sending method

---

## tag v4.1.0
Tagger: nslab RA <nslab@citi.edu.tw>
Date:   Fri Jul 25 11:54:28 2025 +0800

v4.1.0 major changes:

- Change API parameter names ('src_port', 'dst_port', or 'port'), when they don’t denote the flow 5‑tuple ports, to 'interface' for clarity.
- Add 'received_a_simulation_case' and 'simulation_completed' APIs for applications to communicate with the simulation server.
- Fix the 'setPowerStateMininet' bug.
- Add the 'findVertexByMininetBridgeName' function for the intent‑to‑tasks translator module.
- received_a_simulation_case API can get response from simulation server.
- Change NFS server folder authority after application registration.
---

## tag v4.1.1
Tagger: nslab RA <nslab@citi.edu.tw>
Date:   Tue Aug 12 14:43:11 2025 +0800

v4.1.1 major changes:

- Add 'GET /ndt/get_nickname' to retrieve a device's alias by DPID, MAC, or name.
- Add 'POST /ndt/modify_nickname' to update a device's alias.
- Add 'GET /ndt/get_temperature' for polling switch operating temperatures.
- Add 'GET /ndt/get_path_switch_count' to calculate the number of switches between two IP addresses.
- Change to larger topology (10 switches).
---

## tag v4.2.0
Tagger: nslab RA <nslab@citi.edu.tw>
Date:   Mon Aug 25 10:21:01 2025 +0800

Release v4.2.0

1.tag v4.2.0
Tagger: nslab RA <nslab@citi.edu.tw>
Date:   Mon Aug 25 10:21:01 2025 +0800

Release v4.2.0

1. Reparse sFlow datagram for HPE 5520 switch and consider both ingress and egress sampling when calculating average sFlow sending rate.

2. In get_detected_flow_data API, change 'first_sampled_time_ms' and 'latest_sampled_time_ms' to 'first_sampled_time' and 'latest_sampled_time', and return time string.

3. Check whether there are remaining NFS folders for applications.

4. Address CORS issue.

5. Add a new API, install_flow_entries_modify_flow_entries_and_delete_flow_entries, to install/modify/delete flow entries at once (see ndt_api.md).
---


## tag v4.3.0

Tagger: nslab RA [nslab@citi.edu.tw](mailto:nslab@citi.edu.tw)
Date:   Thu Aug 28 10:21:01 2025 +0800

Release v4.3.0



1. Add new APIs for OpenFlow **group entries**:

   * **POST `/ndt/install_group_entry`**: Install a new OpenFlow group entry in a switch.
   * **POST `/ndt/delete_group_entry`**: Delete a group entry from a switch.
   * **POST `/ndt/modify_group_entry`**: Modify an existing group entry in a switch.

2. Add new APIs for OpenFlow **meter entries**:

   * **POST `/ndt/install_meter_entry`**: Install a new meter entry in a switch.
   * **POST `/ndt/delete_meter_entry`**: Delete a meter entry from a switch.
   * **POST `/ndt/modify_meter_entry`**: Modify an existing meter entry in a switch.

3. Add **GET `/ndt/get_openflow_capacity`** to retrieve supported OpenFlow capabilities (groups, meters, tables, etc.) from switches.

4. Add **GET `/ndt/historical_logging`** with query parameter `state=enable|disable` to enable or disable historical data logging.

5. Update **GET `/ndt/get_path_switch_count`**. If omitted source and destination IPs, all paths counts will be returned.

6. Fix `get_openflow_capacity` API output.

7. Fix `PurgeIdleFlows`, `flow set`, `flow sending rate` bug.

8. ICMP parsing. For ICMP flows, the 5-tuple reuses the "port" fields: src_port -> ICMP type, dst_port -> ICMP code. For non-ICMP flows, src_port/dst_port keep their usual meaning. (see ndt_api.md)
---
