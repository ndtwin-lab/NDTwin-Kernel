# Thirdparty RESTful API Documentation
## 1. POST /fair_share_path/choose_path
### Description
Selects the best path for a new flow based on current topology and flow statistics.
### Request
* Content-Type: **application/json**
* Body
```json
{
  "src_ip": 325363904,
  "dst_ip": 56928448,
  "all_paths": [
        [
            [325363904, 3], 
            [106225808391692, 23], 
            [106225808387660, 23], 
            [106225808402492, 3], 
            [56928448, 0]
        ], 
        [
            [325363904, 3], 
            [106225808391692, 24], 
            [106225808380928, 23], 
            [106225808402492, 3], 
            [56928448, 0]
        ],
        ...
    ]
}
```
### Response
#### Success
* Status: **200 OK**
* Body
```json
{
  "path": [
        [325363904, 3], 
        [106225808391692, 24], 
        [106225808380928, 23], 
        [106225808402492, 3], 
        [56928448, 0]
    ]
}
```
#### Error
* Status: **400 Bad Request**
```json
{
    "error": "Invalid JSON payload"
}
```
* Status: **400 Bad Request**
```json
{
    "error": "Missing src_ip, dst_ip or all_paths"
}
```
* Status: **404 Not Found**
```json
{
  "error": "Failed to fetch graph data: ..."
}
```

## 2. POST /fair_share_path/balance_traffic

### Description
Handles dynamic traffic engineering by determining whether flow migration is necessary based on the current network load.
### Request
* Content-Type: **application/json**
* Body
```json
{
  "flow_key": {
    "src_ip": 325363904,
    "dst_ip": 56928448
  }
}
```
### Response
#### Success
* Status: **200 OK**
*  Body (if migration is needed)
```json
{
  "migrated_flow_vector": [
    [
        [
            40151232,
            308586688
        ],
        [
            [
                106225808402492,
                23
            ],
            [
                106225808387660,
                24
            ],
            [
                106225808391692,
                1
            ]
        ]
    ],
    ...
  ]
}
```
* Status: **200 OK**
*  Body (no migration needed)
```json
{
  "status": "no migration needed"
}
```

#### Error
* Status: **400 Bad Request**
```json
{
    "error": "Invalid JSON payload"
}
```
* Status: **400 Bad Request**
```json
{
    "error": "Missing flow_key with src_ip and dst_ip"
}
```
* Status: **404 Not Found**
```json
{
  "error": "Failed to fetch graph or flow data: ..."
}
```
* Status: **404 Not Found**
```json
{
  "error": "Invalid or missing original path"
}
```
* Status: **404 Not Found**
```json
{
  "error": "No path found"
}
```