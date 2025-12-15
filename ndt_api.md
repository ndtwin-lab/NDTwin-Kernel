# NDT RESTful API Documentation
## 1. [Deprecated] POST /ndt/flow_added
### Description
Receives a list of candidate paths for a new flow and emits a **FlowAdded** event. Responds with the selected path or a message if the flow was already installed.
### Request
* Content-Type: **application/json**
* Body
```json
{
  "all_paths": [
    [
      [56928448, 1],
      [106225808402492, 23],
      [106225808387660, 24], 
      [106225808391692, 1],
      [325363904, 0]
    ],
    ...
  ]
}
```
### Response
#### Success
* Status: **200 OK**
* Body (if a new path is selected)
```json
{
  "status": "path selected",
  "path": [
    [56928448, 1],
    [106225808402492, 23],
    [106225808387660, 24],
    [106225808391692, 1],
    [325363904, 0]
  ]
}
```
* Body (if flow is already installed)
```json
{
  "status": "flow already installed"
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "Invalid PacketInPayload format"
}
```
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```


## 2. POST /ndt/link_failure_detected
### Description
Handles link failure notification. Marks the forward and reverse edges in the topology as down and emits **LinkFailureDetected** events.
### Request
* Content-Type: **application/json**
* Body (the information of failed link)
```json
{
  "src_dpid": 106225808402492,
  "src_interface": 23,
  "dst_dpid": 106225808387660,
  "dst_interface": 23
}
```
### Response
#### Success
* Status: **200 OK**
```json
{
  "status": "link failure processed"
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "Invalid link-failure payload"
}
```
* Status: **404 Not Found**
```json
{
  "error": "edge not found in topology"
}
```
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```


## 3. POST /ndt/link_recovery_detected
### Description
Handles link recovery events. Marks the forward and reverse edges in the topology as up.
### Request
* Content-Type: **application/json**
* Body (the information of recovered link)
```json
{
  "src_dpid": 106225808402492,
  "src_interface": 23,
  "dst_dpid": 106225808387660,
  "dst_interface": 23
}
```
### Response
#### Success
* Status: **200 OK**
```json
{
  "status": "link recovery processed"
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "Missing src_dpid or dst_dpid or src_interface or dst_interface"
}
```
* Status: **404 Not Found**
```json
{
  "error": "edge not found in topology"
}
```
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```


## 4. GET /ndt/get_graph_data
### Description
Returns the complete graph topology configured in StaticNetworkTopology.json including nodes and edges, flow information, and edge status.
**vertex_type = 0** means a switch, and **vertex_type = 1** means a host.
At the edge between the switch and host, the dpid and interface on the host side are set to 0.
### Request
* Method: **GET**
### Response
#### Success
* Status: **200 OK**
```json
{
  "nodes": [
    {
      "device_name": "s4",
      "dpid": 106225808380928,
      "ip": [
        168430090
      ],
      "is_enabled": true,
      "is_up": true,
      "mac": 0,
      "vertex_type": 0,
      "brand_name": "OVS",
      "device_layer": 1
    },
    {
      "device_name": "h9",
      "dpid": 0,
      "ip": [
        1157736640,
        1174513856,
        1107404992,
        1090627776,
        1191291072,
        1124182208,
        1140959424,
        1208068288
      ],
      "is_enabled": true,
      "is_up": true,
      "mac": 31362038109890,
      "vertex_type": 1,
      "brand_name": "",
      "device_layer": 3
    },
    ...
  ],
  "edges": [
    {
      "dst_dpid": 0,
      "dst_ip": [
        50440384,
        33663168,
        100772032,
        117549248,
        134326464,
        83994816,
        67217600,
        16885952
      ],
      "dst_interface": 0,
      "flow_set": [
        {
          "dst_ip": 2147592384,
          "dst_port": 5201,
          "protocol_number": 6,
          "src_ip": 16885952,
          "src_port": 40997
        }
      ],
      "is_enabled": true,
      "is_up": true,
      "left_link_bandwidth_bps": 998396604,
      "link_bandwidth_bps": 1000000000,
      "link_bandwidth_usage_bps": 1603396,
      "link_bandwidth_utilization_percent": 0.16033960000000347,
      "src_dpid": 106225808402492,
      "src_ip": [
        67766794
      ],
      "src_interface": 1
    },
    ...
  ]
}
```
#### Error
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```


## 5. GET /ndt/get_detected_flow_data
### Description
Returns detailed information about all active flows observed by the network system (sFLow), including their estimated sending rates, timestamps, protocol type, and the full path taken across the network.

**Note:** For ICMP packets, the src_port field represents the ICMP type, and the dst_port field represents the ICMP code.
### Request
* Method: **GET**
### Response
#### Success
* Status: **200 OK**
```json
[
    {
        "dst_ip": 16885952,
        "dst_port": 55367,
        "estimated_flow_sending_rate_bps_in_the_last_sec": 1712000,
        "estimated_flow_sending_rate_bps_in_the_proceeding_1sec_timeslot": 1817600,
        "estimated_packet_rate_in_the_last_sec": 3000,
        "estimated_packet_rate_in_the_proceeding_1sec_timeslot": 3200,
        "first_sampled_time": "2025-08-22 10:13:12",
        "latest_sampled_time": "2025-08-22 10:13:17",
        "path": [
            {
                "interface": 5,
                "node": 1359063232
            },
            {
                "interface": 22,
                "node": 106225808391692
            },
            {
                "interface": 65,
                "node": 106225808398208
            },
            {
                "interface": 25,
                "node": 897475217989184
            },
            {
                "interface": 21,
                "node": 106225808403428
            },
            {
                "interface": 1,
                "node": 106225808402492
            },
            {
                "interface": 0,
                "node": 16885952
            }
        ],
        "protocol_id": 6,
        "src_ip": 1359063232,
        "src_port": 5201
    },
    {
        "dst_ip": 1359063232,
        "dst_port": 5201,
        "estimated_flow_sending_rate_bps_in_the_last_sec": 988560000,
        "estimated_flow_sending_rate_bps_in_the_proceeding_1sec_timeslot": 962242666,
        "estimated_packet_rate_in_the_last_sec": 81333,
        "estimated_packet_rate_in_the_proceeding_1sec_timeslot": 79166,
        "first_sampled_time": "2025-08-22 10:13:12",
        "latest_sampled_time": "2025-08-22 10:13:18",
        "path": [
            {
                "interface": 1,
                "node": 16885952
            },
            {
                "interface": 22,
                "node": 106225808402492
            },
            {
                "interface": 65,
                "node": 106225803167444
            },
            {
                "interface": 27,
                "node": 897475217989184
            },
            {
                "interface": 21,
                "node": 106225808384924
            },
            {
                "interface": 5,
                "node": 106225808391692
            },
            {
                "interface": 0,
                "node": 1359063232
            }
        ],
        "protocol_id": 6,
        "src_ip": 16885952,
        "src_port": 55367
    }
]
```
#### Error
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```


## 6. GET /ndt/get_switch_openflow_table_entries
### Description
Returns raw OpenFlow flow table entries directly from each switch in the topology. 

Each entry includes the DPID of the switch and the exact OpenFlow flow entries as reported by that switch.


### Request
* Method: **GET**
### Response
#### Success
* Status: **200 OK**
```json
[
  {
    "dpid": 106225808402492,
    "flows": {
      "106225808402492": [
        {
          "actions": [
            "OUTPUT:1"
          ],
          "byte_count": 0,
          "cookie": 0,
          "duration_nsec": 91000000,
          "duration_sec": 3935,
          "flags": 0,
          "hard_timeout": 0,
          "idle_timeout": 0,
          "length": 96,
          "match": {
            "dl_type": 2048,
            "nw_dst": "192.168.1.1"
          },
          "packet_count": 0,
          "priority": 10,
          "table_id": 0
        },
        ...
      ]
    }
  }
]
```
#### Error
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```

## 7. GET /ndt/get_power_report
### Description
Returns the estimated power consumption (in milliwatts, mW) of each switch over the past five minutes. The behavior varies based on deployment mode:

In MININET mode, random values are generated for demonstration purposes.

In TESTBED mode, power values are collected via SSH from each switch's IP address and parsed from the returned output.


### Request
* Method: **GET**
### Response
#### Success
* Status: **200 OK**
```json
[
  {
    "dpid": 106225808391692,
    "power_consumed": 851157966
  },
  {
    "dpid": 106225808380928,
    "power_consumed": 851152638
  },
  {
    "dpid": 106225808387660,
    "power_consumed": 842764030
  },
  {
    "dpid": 106225808402492,
    "power_consumed": 851157966
  }
]
```
#### Error
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```


## 8. POST /ndt/disable_switch
### Description
Executes a simulation to recalculate all-destination routing entries while excluding the disabled switch using hash routing policy and returns the differences in the OpenFlow table for each switch.

Disables the switch and the links connected to it (sets is_enabled to false).


### Request
* Content-Type: **application/json**
* Body
```json
{
  "dpid": 106225808387660
}
```
### Response
#### Success
* Status: **200 OK**
```json
[
  {
    "dpid": 106225808391692,
    "modified": [
      {
        "dst_ip": 939632832,
        "new_output_interface": 24,
        "old_output_interface": 23
      },
      ...
    ]
  },
  {
    "dpid": 106225808387660,
    "removed": [
      {
        "dst_ip": 16885952,
        "old_output_interface": 23
      },
      ...
    ]
  },
  ...
]
```
#### Error
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```
* **Status: 400 Bad Request**
```json
{
  "error": "Missing dpid"
}
```
* **Status: 404 Not Found**
```json
{
  "error": "Switch not found"
}
```


## 9. POST /ndt/enable_switch
### Description
Re-enables a previously disabled switch and all its associated links in the network topology.


### Request
* Content-Type: **application/json**
* Body
```json
{
  "dpid": 106225808387660
}
```
### Response
#### Success
* Status: **200 OK**
```json
{
  "status": "enable switch processed"
}
```
#### Error
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```
* **Status: 400 Bad Request**
```json
{
  "error": "Missing dpid"
}
```
* **Status: 404 Not Found**
```json
{
  "error": "Switch not found"
}
```


## 10. [Deprecated] POST /ndt/install_a_new_path_for_a_flow
### Description
Installs a new path for a specific (existing) flow by specifying the source and destination IPs, along with a list of **[dpid/ip, interface]** pairs representing the full path. This API calls the internal **migrateAFlow()** method to reroute the flow according to the provided path.


### Request
* Content-Type: **application/json**
* Body
```json
{
  "src_ip": 23374016, 
  "dst_ip": 291809472, 
  "path": [
      [
        23374016,
        0
      ],
      [
        106225808402492,
        24
      ],
      [
        106225808380928,
        24
      ],
      [
        106225808391692,
        0
      ],
      [
        291809472,
        0
      ]
  ]
}
```
### Response
#### Success
* Status: **200 OK**
```json
{
  "status": "Flow path installed successfully"
}
```
#### Error
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```
* **Status: 400 Bad Request**
```json
{
  "error": "Missing src_ip, dst_ip, or path"
}
```
* **Status: 400 Bad Request**
```json
{
  "error": "Each path element must be an array of [dpid/ip, interface]"
}
```


## 11. GET /ndt/get_switches_power_state

### Description
Query the power state of one or all switches.  
- **With `?ip=`**: returns the state of the specified switch  
- **Without parameters**: returns the state of *all* known switches  

```shell
 GET "http://127.0.0.1:8000/ndt/get_switches_power_state?ip=10.10.10.10"
```

Results are returned as a JSON object where each key is a switch IP and each value is `"ON"` or `"OFF"`.

---

### Request
* Query Parameter:

  ip (optional) — the switch IP to query; if omitted, returns all switches

### Response

#### Success

* Status: **200 OK**

```json
{
  "10.10.10.10": "ON"
}
```

```json
{
  "10.10.10.10": "ON",
  "10.10.10.3":  "OFF",
  "10.10.10.4":  "ON",
  "10.10.10.9":  "OFF"
}
```

#### Error

* **Status: 404 Not Found**

```json
{
  "error": "Unknown switch IP"
}
```

* **Status: 500 Internal Server Error**

```json
{
  "error": "Internal server error"
}
```

## 12. POST /ndt/set_switches_power_state

### Description

Sends a power control command to one of the network switches by interacting with the smart plug associated with it.

### Request

* Content-Type: **application/json**
* Required Query Parameters:

| Parameter | Type   | Description                            |
| --------- | ------ | -------------------------------------- |
| `ip`      | string | IP address of the switch               |
| `action`  | string | Desired power state: `"on"` or `"off"` |

* Body
  None

```shell
POST "http://localhost:8000/ndt/set_switches_power_state?ip=10.10.10.10&action=on"
```

### Response

#### Success

* **Status: 200 OK**

```json
{
  "10.10.10.10": "Success"
}
```

#### Error

* **Status: 500 Internal Server Error**

```json
{
  "error": "Internal server error while processing the power control request"
}
```

* **Status: 400 Bad Request**

```json
{
  "error": "Missing or malformed query parameters"
}
```

## 13. POST /ndt/install_flow_entry
### Description
Installs a new OpenFlow flow entry in a specific switch via the Ryu controller. 

The API constructs and sends a **flowentry/add** POST request to Ryu.

Reconstructs the new path for affected flows and updates the allPathMap.


### Request
* Content-Type: **application/json**
* Body
```json
{
  "dpid": 106225808380928,
  "priority": 99,
  "match": {
    "eth_type": 2048,
    "ipv4_dst": "10.0.0.3"
  },
  "actions": [
    {
      "type": "OUTPUT",
      "port": 24
    }
  ]
}
```
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{
  "status": "Flow installed"
}

```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "<error message>"
}
```


## 14. POST /ndt/delete_flow_entry
### Description
Deletes flow entries from a switch based on match fields. 

Internally sends a **flowentry/delete** request to the Ryu controller.

Reconstructs the new path for affected flows and updates the allPathMap.

### Request
* Content-Type: **application/json**
* Body
```json
{
  "dpid": 106225808380928,
  "match": {
    "eth_type": 2048,
    "ipv4_dst": "10.0.0.3"
  }
}
```
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{
  "status": "Flow deleted"
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "<error message>"
}
```



## 15. POST /ndt/modify_flow_entry
### Description
Modifies an existing flow entry by matching criteria and applying new actions. 

Sends a **flowentry/modify** request to the Ryu controller.

Reconstructs the new path for affected flows and updates the allPathMap.

### Request
* Content-Type: **application/json**
* Body
```json
{
  "dpid": 106225808380928,
  "priority": 99,
  "match": {
    "eth_type": 2048,
    "ipv4_dst": "10.0.0.3"
  },
  "actions": [
    {
      "type": "OUTPUT",
      "port": 25
    }
  ]
}
```
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{
  "status": "Flow modified"
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "<error message>"
}
```


## 16. GET /ndt/get_cpu_utilization
### Description
Returns the current CPU utilization(%) of all up switches in the network topology. Uses SNMP OID **1.3.6.1.4.1.1991.1.1.2.1.52.0** to retrieve CPU data. 

In MININET mode, dummy values are generated for demonstration purposes.

### Request
* Method: **GET**
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{"10.10.10.10":1,"10.10.10.3":1,"10.10.10.4":1,"10.10.10.9":1}
```
A value of -1 means SNMP query failed or data is unavailable.


## 17. GET /ndt/get_memory_utilization
### Description
Returns the memory utilization(%) of all up switches using SNMP OID **1.3.6.1.4.1.1991.1.1.2.1.53.0.**

In MININET mode, dummy values are generated for demonstration purposes.

### Request
* Method: **GET**
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{"10.10.10.10":28,"10.10.10.3":27,"10.10.10.4":27,"10.10.10.9":27}
```
A value of -1 means SNMP query failed or data is unavailable.


## 18. GET /ndt/inform_switch_entered
### Description
Notifies the NDT system that a new switch has entered the network (i.e., connected). This will mark the switch as “up” in the internal topology.

```shell
GET "http://localhost:8000/ndt/inform_switch_entered?dpid=106225808402492"
```
### Request
* Method: **GET**
* Query Parameter: dpid
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{
  "status": "Switch set to up"
}

```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "Missing dpid parameter"
}
```
* Status: **400 Bad Request**
```json
{
  "error": "Invalid dpid format"
}
```
* Status: **404 Not Found**
```json
{
  "error": "Switch not found"
}
```


## 19. POST /ndt/modify_device_name
### Description
Updates the name of a switch or host in the NDT topology and StaticNetworkTopology.json.


### Request
* Content-Type: **application/json**
* Body Parameters:

|     Field     |    Type  |      Required     |         Description                        |
| ------------- | -------- | ----------------- | ------------------------------------------ |
| `vertex_type` | `int`    |        Yes        | `0` for switch, `1` for host               |
|    `dpid`     | `uint64` |  Yes (if switch)  | 	The DPID of the switch to rename        |
|     `mac`     | `string` |  Yes (if host)  | 	The MAC address of the host to rename (e.g., "00:11:22:33:44:55")          |
|  `new_name`   | `string` |        Yes        | 	The new name to assign to the device    |


* Body
```json
{
  "vertex_type": 1,
  "mac": "00:11:22:33:44:55",
  "new_name": "HostA"
}
```
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{
  "status": "Device name updated successfully."
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": 	"Invalid vertex_type. Must be 0 (switch) or 1 (host)."
}
```
* Status: **400 Bad Request**
```json
{
  "error": 	"Invalid request format."
}
```
* Status: **404 Not Found**
```json
{
  "error": 	"Device not found."
}
```

## 20. GET /ndt/get_static_topology_json
### Description
Generates the current topology StaticNetworkTopology content.

The detailed demonstration video is available at the following link:
https://hackmd.io/@pyjuan91/patty

### Request
* Method: **GET**

### Response
#### Success
* Status: **200 OK**
```json
{
    "edges": [
        {
        "dst_dpid": 1,
        "dst_ip": [
            192653504
        ],
        "dst_interface": 2,
        "link_bandwidth_bps": 1000000000,
        "src_dpid": 4,
        "src_ip": [
            242985152
        ],
        "src_interface": 1
        },
        ...
    ],

    "nodes": [
        {
        "brand_name": "OVS",
        "device_layer": 1,
        "device_name": "s4",
        "dpid": 4,
        "ip": [
            242985152
        ],
        "mac": 0,
        "vertex_type": 0
        },

        ...
    ]
}
```

## 21. POST /ndt/app_register
### Description
Registers a new application with the server.
When an application is registered:

- The server assigns it a unique app_id.

- Creates a dedicated folder for the application in the NFS export directory (e.g., /srv/nfs/sim/<app_id>).

- This folder can be used by the application to store its simulation files.

### Request
* Content-Type: **application/json**
* Body
```json
{
  "app_name": "MyApp",
  "simulation_completed_url": "http://192.168.100.5:9000/simulation_completed"
}
```

### Response
#### Success
* Status: **200 OK**
```json
{
  "app_id": 1,
  "message": "Application registered successfully"
}
```

#### Error
* Status: **400 Bad Request**
```json
{
  "error": "Missing or invalid 'appName'"
}
```
* Status: **400 Bad Request**
```json
{
  "error": "Invalid JSON body"
}
```
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```

## 22. POST /ndt/received_a_simulation_case

### Description

Notifies the NDT server that a new simulation case has been dispatched by an external simulator and reply the response from simulation server.

### Request

* **Content-Type:** `application/json`
* **Body Parameters:**

  | Field       | Type   | Description                                                             |
  | ----------- | ------ | ----------------------------------------------------------------------- |
  | `simulator` | string | Name of the simulator (e.g., `"NetSquid"`)                              |
  | `version`   | string | Version identifier of the simulator (e.g., `"v1.2.3"`)                  |
  | `app_id`    | string | Identifier of the registered application (as returned by app\_register) |
  | `case_id`   | string | Unique identifier for this simulation case                              |
  | `inputfile` | string | Path or URL where the simulator can fetch its input description         |

```json
{
  "simulator": "MySimulator",
  "version": "1.0.0",
  "app_id": "1",
  "case_id": "case_123",
  "inputfile": "/srv/nfs/sim/1/case_123.json"
}
```

### Response

#### Success

* **Status:** `202 Accepted`
* **Body:**

  ```json
  {
    "status": "Request received (response from simulation server)"
  }
  ```

#### Error

* **Status:** `400 Bad Request`

  ```json
  {
    "error": "Missing or invalid fields in request body"
  }
  ```
* **Status:** `500 Internal Server Error`

  ```json
  {
    "error": "internal server error"
  }
  ```

---

## 23. POST /ndt/simulation_completed

### Description

Called by the external simulator when a simulation finishes. The server will forward the result URL to the registered application via the SimulationRequestManager’s callback.

### Request

* **Content-Type:** `application/json`
* **Body Parameters:**

  | Field        | Type   | Description                                                   |
  | ------------ | ------ | ------------------------------------------------------------- |
  | `app_id`     | string | Identifier of the application that submitted the simulation   |
  | `case_id`    | string | Identifier of the simulation case that has completed          |
  | `outputfile` | string | Path or URL where the simulator has deposited its output file |

```json
{
  "app_id": "1",
  "case_id": "case_123",
  "outputfile": "/srv/nfs/sim/1/case_123_result.json"
}
```

### Response

#### Success

* **Status:** `200 OK`
* **Body:**

  ```json
  {
    "status": "result forwarded"
  }
  ```

#### Error

* **Status:** `400 Bad Request`

  ```json
  {
    "error": "Missing or invalid fields in request body"
  }
  ```
* **Status:** `500 Internal Server Error`

  ```json
  {
    "error": "internal server error"
  }
  ```


## 24.GET /ndt/get_nickname

-----

### Description

Retrieves the **nickname** of a single device from the network topology. The device must be identified by one of the query parameters: `dpid`, `mac`, or `name`.

If multiple parameters are provided, they are processed in the following order of priority: **`dpid` \> `mac` \> `name`**. For example, if both `dpid` and `mac` are in the URL, the system will only search for the device by its `dpid`.

-----

### Request

  * **Query Parameters**:

    > At least one of the following parameters is required.

    | Parameter | Type | Description |
    | :--- | :--- | :--- |
    | `dpid` | `uint64` | The **DPID** of the switch to find. |
    | `mac` | `string` | The **MAC address** of the device to find (e.g., "00:1A:2B:3C:4D:5E"). |
    | `name` | `string` | The current **device name** to find. |

  * **Example URLs**:

      * To get by DPID:
        `/api/device/nickname?dpid=4660`
      * To get by MAC address:
        `/api/device/nickname?mac=00:1A:2B:3C:4D:5E`

-----

### Response

#### Success ✅

  * **Status**: `200 OK`

  * **Body**: A JSON object containing the nickname of the found device.

    ```json
    {
      "nickname": "Main-Web-Server"
    }
    ```

#### Error ❌

  * **Status**: `404 Not Found`

    > Returned when a valid identifier is provided, but no matching device is found in the topology.

    ```json
    {
      "error": "Device not found"
    }
    ```

  * **Status**: `400 Bad Request`

    > Returned if no identifier parameter (`dpid`, `mac`, or `name`) is provided in the URL.

    ```json
    {
      "error": "Missing dpid, mac, or name parameter"
    }
    ```

  * **Status**: `400 Bad Request`

    > Returned if an identifier has an invalid format (e.g., a non-numeric DPID).

    ```json
    {
      "error": "Invalid DPID format",
      "details": "stoull"
    }
    ```

## 25. POST /ndt/modify_nickname

-----

### Description

Updates the nickname of a device (e.g., a switch or host) in the network topology. The device can be identified by its **DPID**, **MAC address**, or its current **name**(e.g., name or nickname).

-----

### Request

  * **Content-Type**: `application/json`

  * **Body Parameters**:

| Field | Type | Required | Description |
| :--- | :--- | :--- | :--- |
| `identifier` | `object` | Yes | An object containing the unique identifier for the device. |
| `↳ identifier.type` | `string` | Yes | Specifies the type of identifier. Must be one of: `"dpid"`, `"mac"`, or `"name"`. |
| `↳ identifier.value`| `int`,`uint64` or `string` | Yes | The value corresponding to the identifier type (e.g., a DPID for `dpid`, a MAC address string for `mac`). |
| `new_nickname` | `string` | Yes | The new nickname to assign to the device. |
  
  * **Body (Example by DPID)**

    ```json
    {
      "identifier": {
        "type": "dpid",
        "value": 4660
      },
      "new_nickname": "Sinica-Switch-01"
    }
    ```

  * **Body (Example by MAC)**

    ```json
    {
      "identifier": {
        "type": "mac",
        "value": "00:1A:2B:3C:4D:5E"
      },
      "new_nickname": "Main-Web-Server"
    }
    ```
  * **Body (Example by NAME)**

    ```json
    {
      "identifier": {
        "type": "name",
        "value": "h1"
      },
      "new_nickname": "Core-Switch-01"
    }
    ```

-----

### Response

#### Success ✅

  * **Status**: `200 OK`
  * **Body**
    ```json
    {
      "status": "success",
      "message": "Nickname updated successfully."
    }
    ```

#### Error ❌

  * **Status**: `404 Not Found`

    ```json
    {
      "error": "Device not found"
    }
    ```

  * **Status**: `400 Bad Request`

    > This error is returned for invalid request formats, such as malformed JSON, missing required fields, or an invalid identifier type.

    ```json
    {
      "error": "Failed to modify nickname",
      "details": "Invalid identifier type: ip"
    }
    ```
Of course. Here is the API documentation for your temperature function, written in the same style as your example.

-----

## 26. GET /ndt/get_temperature

**Description**

Retrieves the current operating temperature for all switches in the network topology.

This function is designed to work in two modes:

  * In a **Mininet** environment, it returns randomly generated dummy data for any switch.
  * In a **Testbed** environment, it uses SNMP to query the temperature. It will only return a valid temperature for devices with a `brand_name` of "HPE 5520". For other devices or switches that are down, it returns a descriptive status message.

-----

### Request

**Query Parameters:**

This endpoint does not take any query parameters.

**Example URL:**

```
/ndt/get_temperature
```

-----

### Response

#### Success ✅

**Status**: `200 OK`

**Body**: A JSON object where each key is a switch's IP address. The value is either the current temperature in Celsius (as an integer) or a status message (as a string).

```json
{
  "10.10.10.15": "The temperature function only supports the HPE 5520.",
  "10.10.10.16": 29,
  "10.10.10.17": "The switch is down."
}
```

-----

#### Error ❌

No specific error responses are defined for this endpoint. The status of each individual switch (e.g., if it's down or unsupported) is reported within the body of a `200 OK` success response, as shown above. Any critical server-side failure would result in a standard `500 Internal Server Error`.



Of course. Here is the updated API documentation that illustrates how the endpoint works when the `src_ip` and `dst_ip` parameters are omitted.

The main change is that providing these parameters is now **optional**. If you omit them, the API returns a complete list of all known paths and their switch counts.

-----

## 27. GET /ndt/get_path_switch_count

-----

### Description

Retrieves the number of switches along network paths.

  * If a **source IP** and **destination IP** are specified, it returns the switch count for that single path.
  * If no IP addresses are specified, it returns a list of all known paths and their corresponding switch counts.

-----

### Request

  * **Query Parameters**:

| Parameter | Type     | Required | Description                                                                                 |
| :-------- | :------- | :------- | :------------------------------------------------------------------------------------------ |
| `src_ip`  | `string` | No       | The source IP address of the path (e.g., "10.0.0.1"). If omitted, all paths will be returned. |
| `dst_ip`  | `string` | No       | The destination IP address of the path (e.g., "10.0.0.2"). If omitted, all paths will be returned. |

  * **Example URLs**:
      * **For a specific path**:
        `/ndt/get_path_switch_count?src_ip=10.0.0.1&dst_ip=10.0.0.2`
      * **For all paths (empty query)**:
        `/ndt/get_path_switch_count`

-----

### Response

#### Success (Specific Path) ✅

  * **Status**: `200 OK`

  * **Body**: A JSON object confirming the request and providing the count of switches on the specified path.

    ```json
    {
      "status": "success",
      "src_ip": "10.0.0.1",
      "dst_ip": "10.0.0.2",
      "switch_count": 1
    }
    ```

#### Success (All Paths) ✅

  * **Status**: `200 OK`

  * **Body**: A JSON object containing a `data` array with all known paths and their switch counts. The array will be empty if no paths are currently known.

    ```json
    {
      "status": "success",
      "data": [
        {
          "src_ip": "10.0.0.1",
          "dst_ip": "10.0.0.2",
          "switch_count": 1
        },
        {
          "src_ip": "10.0.0.3",
          "dst_ip": "10.0.0.4",
          "switch_count": 2
        }
      ]
    }
    ```

#### Error ❌

  * **Status**: `404 Not Found`

    > This error is returned **only when requesting a specific path** that cannot be found in the system's records.

    ```json
    {
      "status": "error",
      "message": "Path not found for the given IPs."
    }
    ```

## 28. POST /ndt/install_flow_entries_modify_flow_entries_and_delete_flow_entries

### Description

Installs, modifies and deletes OpenFlow entries in one request. For each installation/modification/deletion, the server updates rules and recalculates affected paths, then returns a success status.

### Request

* **Content-Type:** `application/json`
* **Body Parameters:**

  | Field        | Type   | Description                                                   |
  | ------------ | ------ | ------------------------------------------------------------- |
  | `install_flow_entries`     | array | Array of install entries. Each item must include dpid, match, and actions; priority optional (0 as default value).   |
  | `modify_flow_entries`     | array | Array of modify entries. Each item must include dpid, match, and actions; priority optional (0 as default value).   |
  | `delete_flow_entries`    | array | Array of delete entries. Each item must include dpid and match.          |

* **Install/Modify entry fields**

  | Field        | Type   | Description                                                   |
  | ------------ | ------ | ------------------------------------------------------------- |
  | `dpid`     | integer | Switch datapath ID (uint64).   |
  | `priority`    | integer | 	Rule priority (optional; defaults to 0 if missing).          |
  | `match`    | object | 	Match fields.          |
  | `actions`    | array | 	List of actions.          |

* **Delete entry fields**
  | Field        | Type   | Description                                                   |
  | ------------ | ------ | ------------------------------------------------------------- |
  | `dpid`     | integer | Switch datapath ID (uint64).   |
  | `match`    | object | 	Match fields.          |


```json
{
    "install_flow_entries":
    [
        {
            "dpid": 106225808387660,
            "priority": 99,
            "match": {
                "eth_type": 2048,
                "ipv4_dst": "10.0.0.3"
            },
            "actions": [
                {
                "type": "OUTPUT",
                "port": 21
                }
            ]
        }
    ],
    "modify_flow_entries":
    [
        {
            "dpid": 106225808387660,
            "priority": 10,
            "match": {
                "eth_type": 2048,
                "ipv4_dst": "192.168.1.1"
            },
            "actions": [
                {
                "type": "OUTPUT",
                "port": 1
                }
            ]
        },
        {
            "dpid": 106225808387660,
            "priority": 10,
            "match": {
                "eth_type": 2048,
                "ipv4_dst": "192.168.1.2"
            },
            "actions": [
                {
                "type": "OUTPUT",
                "port": 1
                }
            ]
        },
        {
            "dpid": 106225808402492,
            "priority": 10,
            "match": {
                "eth_type": 2048,
                "ipv4_dst": "192.168.1.1"
            },
            "actions": [
                {
                "type": "OUTPUT",
                "port": 1
                }
            ]
        }
    ],
    "delete_flow_entries":
    [
        
        {
            "dpid": 106225808387660,
            "match": {
                "eth_type": 2048,
                "ipv4_dst": "192.168.1.3"
            }
        }
    ]
}
```

### Response

#### Success

* Status: **200 OK**
* **Body:**

  ```json
  {
    "status": "Flows installed, modified and deleted"
  }
  ```

#### Error

* Status: **400 Bad Request**

  ```json
  {
    "error": "install_flow_entries/modify_flow_entries/delete_flow_entries must be arrays"
  }
  ```
  ```json
  {
    "error": "Bad entry"
  }
  ```
* **Status: 500 Internal Server Error**

  ```json
  {
    "error": "internal server error"
  }
  ```

## 29. POST /ndt/install_group_entry
### Description
Installs a new OpenFlow group entry in a specific switch via the Ryu controller. 

The API constructs and sends a **groupentry/add** POST request to Ryu.

### Request
* Content-Type: **application/json**
* Body
```json
{
    "dpid": 334525264558512,
    "type": "ALL",
    "group_id": 10,
    "buckets": [
      { "actions": [ { "type": "OUTPUT", "port": 25 } ] },
      { "actions": [ { "type": "OUTPUT", "port": 26 } ] }
    ]
}
```
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{
  "status":"Group entry installed"
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "<error message>"
}
```
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```


## 30. POST /ndt/delete_group_entry
### Description
Deletes a new OpenFlow group entry in a specific switch via the Ryu controller. 

The API constructs and sends a **groupentry/delete** POST request to Ryu.

### Request
* Content-Type: **application/json**
* Body
```json
{
    "dpid": 106225808398208	,
    "group_id": 10,
    "type": "ALL"
}
```
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{
  "status":"Group entry deleted"
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "<error message>"
}
```
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```

## 31. POST /ndt/modify_group_entry
### Description
Modifies a new OpenFlow group entry in a specific switch via the Ryu controller. 

The API constructs and sends a **groupentry/modify** POST request to Ryu.

### Request
* Content-Type: **application/json**
* Body
```json
{
    "dpid": 106225808398208	,
    "type": "INDIRECT",
    "group_id": 20,
    "buckets": [
      { "actions": [ { "type": "OUTPUT", "port": 22 } ] }
    ]
}
```
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{
  "status":"Group entry modified"
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "<error message>"
}
```
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```

## 32. POST /ndt/install_meter_entry
### Description
Installs a new OpenFlow meter entry in a specific switch via the Ryu controller. 

The API constructs and sends a **meterentry/add** POST request to Ryu.

### Request
* Content-Type: **application/json**
* Body
```json
{
    "dpid": 106225808398208,
    "flags": ["KBPS", "BURST", "STATS"],
    "meter_id": 5,
    "bands": [
      { "type": "DROP", "rate": 5000, "burst_size": 1000 }
    ]
}
```
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{
  "status":"Meter entry installed"
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "<error message>"
}
```
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```

## 33. POST /ndt/delete_meter_entry
### Description
Deletes a new OpenFlow meter entry in a specific switch via the Ryu controller. 

The API constructs and sends a **meterentry/delete** POST request to Ryu.

### Request
* Content-Type: **application/json**
* Body
```json
{
    "dpid": 106225808398208,
    "meter_id": 5
}
```
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{
  "status":"Meter entry deleted"
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "<error message>"
}
```
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```

## 34. POST /ndt/modify_meter_entry
### Description
Modifies a new OpenFlow meter entry in a specific switch via the Ryu controller. 

The API constructs and sends a **meterentry/modify** POST request to Ryu.

### Request
* Content-Type: **application/json**
* Body
```json
{
    "dpid": 106225808398208,
    "flags": ["KBPS", "BURST", "STATS"],
    "meter_id": 5,
    "bands": [
      { "type": "DROP", "rate": 10000, "burst_size": 2000 }
    ]
}
```
### Response
#### Success
* Status: **200 OK**
* Body 
```json
{
  "status":"Meter entry modified"
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "<error message>"
}
```
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```


## 35. GET /ndt/get_openflow_capacity
### Description

### Request
* Method: **GET**
### Response
#### Success
* Status: **200 OK**
```json
{
    "BrocadeICX7250": {
        "groups": {
            "types": [
                "ALL",
                "INDIRECT",
                "FF"
            ]
        },
        "meters": {
            "bands": [
                "DROP"
            ],
            "flags": [
                "KBPS",
                "BURST",
                "STATS"
            ],
            "supported": true
        },
        "openflow_versions": [
            "1.0",
            "1.3"
        ],
        "tables": [
            {
                "actions": [
                    "OUTPUT",
                    "SET_FIELD",
                    "DROP",
                    "GROUP"
                ],
                "id": 0,
                "instructions": [
                    "APPLY_ACTIONS",
                    "WRITE_ACTIONS",
                    "METER"
                ],
                "matches": [
                    "in_port",
                    "eth_src",
                    "eth_dst",
                    "eth_type",
                    "ipv4_dst"
                ],
                "max_entries": 3072
            }
        ]
    },
    ...
}
```
#### Error
* Status: **400 Bad Request**
```json
{
  "error": "<error message>"
}
```
* **Status: 500 Internal Server Error**
```json
{
  "error": "internal server error"
}
```

## 36. GET /ndt/historical_logging
### Description
Enable or disable historical logging in the system.
The state is set by passing the query parameter state with value enable or disable.



### Request
* Method: **GET**
* URL Parameters:
    * state (required): must be either "enable" or "disable"
### Example
```shell
 GET "http://127.0.0.1:8000/ndt/historical_logging?state=enable"
```
### Response
#### Success
* Status: **200 OK**
```json
{
  "status": "success",
  "message": "Historical data logging has been enabled."
}
```

#### Error
* Status: **400 Bad Request**
```json
{
  "error": "Invalid or missing 'state' parameter. Use 'enable' or 'disable'."
}
```
* **Status: 500 Internal Server Error**
```json
{
  "status": "error",
  "message": "Historical data manager not available."
}
```


## 37. GET /ndt/historical_logging

-----

### Description

Enables or disables the system-wide historical data logging feature. The desired state is controlled via a query parameter.

-----

### Request

  * **Query Parameters**:

| Parameter | Type | Required | Description |
| :--- | :--- | :--- | :--- |
| `state` | `string` | Yes | The desired logging state. Must be one of: `"enable"` or `"disable"`. |

  * **Example URLs**:
      * **To Enable Logging**:
        `/ndt/historical_logging?state=enable`
      * **To Disable Logging**:
        `/ndt/historical_logging?state=disable`

-----

### Response

#### Success ✅

  * **Status**: `200 OK`
  * **Body**
    ```json
    {
      "status": "success",
      "message": "Historical data logging has been enabled."
    }
    ```
    or
    ```json
    {
      "status": "success",
      "message": "Historical data logging has been disabled."
    }
    ```

#### Error ❌

  * **Status**: `400 Bad Request`
    ```json
    {
      "error": "Invalid or missing 'state' parameter. Use 'enable' or 'disable'."
    }
    ```
  * **Status**: `500 Internal Server Error`
    ```json
    {
      "status": "error",
      "message": "Historical data manager not available."
    }
    ```