# hello-parodus

Example illustrating simple event interaction with parodus.

[![Apache V2 License](http://img.shields.io/badge/license-Apache%20V2-blue.svg)](https://github.com/Comcast/libparodus/blob/master/LICENSE.txt)

# Building Instructions

```
mkdir build
cd build
cmake ..
make
```

# Running the application
1. Ensure parodus is running first successfully.
2. Start the application after building, like so - ```./src/hello-parodus -p <parodus local URL> -c <URL to receive parodus response>``` 
3. From a separate terminal, send the following curl command - ```curl -i -d '{"message": "Hello Parodus"}' -X POST https://api.webpa.comcast.net:8090/api/v2/device/mac:<mac address provided to parodus>/iot -H "Authorization: Bearer <valid token>"

# Expected behavior
1. The Parodus debug should be something similar to the following:-
```[1503345617][PARODUS][Info]: Received msg from server:payload{"message": "Hello Parodus"}include_spansspansmsg_typetransaction_uuid$5a9200dd-86ab-11e7-9cc7-fa163e4bb9ecsource!dns:scytale.webpa.comcast.net/iotdestmac:123456789012/iotcontent_type!application/x-www-form-urlencoded
[1503345617][WRP-C][Debug]: unpacking encoded data
[1503345617][WRP-C][Debug]: unpack_ret:2
[1503345617][WRP-C][Debug]: Binary payload {"message": "Hello Parodus"}
[1503345617][WRP-C][Debug]: spans is nil
[1503345617][PARODUS][Debug]: 
Decoded recivedMsg of size:240
[1503345617][PARODUS][Info]: msgType received:3
[1503345617][PARODUS][Debug]: numOfClients registered is 1
[1503345617][PARODUS][Debug]: ********* validate_partner_id ********
[1503345617][PARODUS][Debug]: partner_ids list is NULL
[1503345617][PARODUS][Info]: Received downstream dest as :iot
[1503345617][PARODUS][Debug]: node is pointing to temp->service_name iot 
[1503345617][PARODUS][Debug]: sending to nanomsg client iot
[1503345617][PARODUS][Info]: sent downstream message 'payload{"message": "Hello Parodus"}include_spansspansmsg_typetransaction_uuid$5a9200dd-86ab-11e7-9cc7-fa163e4bb9ecsource!dns:scytale.webpa.comcast.net/iotdestmac:123456789012/iotcontent_type!application/x-www-form-urlencoded' to reg_client 'tcp://172.20.5.2:6668'
[1503345617][PARODUS][Debug]: downstream bytes sent:240
[1503345617][PARODUS][Debug]: free for downstream decoded msg
[1503345617][PARODUS][Debug]: Mutex destroyed 
[1503345617][PARODUS][Debug]: mutex lock in consumer thread
[1503345617][PARODUS][Debug]: Before pthread cond wait in consumer thread
[1503345617][PARODUS][Info]: Upstream message received from nanomsg client: "msg_typesourcemac:123456789012/iotdest!dns:scytale.webpa.comcast.net/iottransaction_uuid$5a9200dd-86ab-11e7-9cc7-fa163e4bb9eccontent_typeapplication/jsoninclude_spanspayload2{"device_id":"mac:PCApplication","iot":"response"}"
[1503345617][PARODUS][Debug]: Producer added message
[1503345617][PARODUS][Debug]: mutex unlock in producer thread
[1503345617][PARODUS][Info]: nanomsg server gone into the listening mode...
[1503345617][PARODUS][Debug]: mutex unlock in consumer thread after cond wait
[1503345617][PARODUS][Debug]: mutex lock in consumer thread
[1503345617][PARODUS][Debug]: mutex unlock in consumer thread
[1503345617][PARODUS][Debug]: ---- Decoding Upstream Msg ----
[1503345617][WRP-C][Debug]: unpacking encoded data
[1503345617][WRP-C][Debug]: unpack_ret:2
[1503345617][WRP-C][Debug]: Binary payload {"device_id":"mac:PCApplication","iot":"response"}
[1503345617][PARODUS][Info]:  Received upstream data with MsgType: 3
```
2. The debug output of the hello-parodus-request-response app should be similar to the following:-
```[1503345602][hello-parodus][Debug]: Connect parodus, etc. 
[1503345602][hello-parodus][Info]: max_retry_sleep is 31
[1503345602][hello-parodus][Info]: Configurations => service_name : iot parodus_url : tcp://172.20.5.2:6666 client_url : tcp://172.20.5.2:6668
[1503345602][hello-parodus][Debug]: New backoffRetryTime value calculated as 3 seconds
[1503345602][WRP-C][Debug]: ***   Start of Msgpack Encoding  ***
[1503345602][hello-parodus][Info]: Init for parodus Success..!!
[1503345602][hello-parodus][Debug]: Parodus Receive wait thread created Successfully -569293056
[1503345602][hello-parodus][Debug]: parodus_receive_wait
[1503345602][WRP-C][Debug]: unpacking encoded data
[1503345602][WRP-C][Debug]: unpack_ret:2
[1503345604][hello-parodus][Debug]:     rtn = 1
[1503345604][hello-parodus][Info]: Timed out or message closed.
[1503345606][hello-parodus][Debug]:     rtn = 1
[1503345606][hello-parodus][Info]: Timed out or message closed.
[1503345608][hello-parodus][Debug]:     rtn = 1
[1503345608][hello-parodus][Info]: Timed out or message closed.
[1503345610][hello-parodus][Debug]:     rtn = 1
[1503345610][hello-parodus][Info]: Timed out or message closed.
[1503345612][hello-parodus][Debug]:     rtn = 1
[1503345612][hello-parodus][Info]: Timed out or message closed.
[1503345614][hello-parodus][Debug]:     rtn = 1
[1503345614][hello-parodus][Info]: Timed out or message closed.
[1503345616][hello-parodus][Debug]:     rtn = 1
[1503345616][hello-parodus][Info]: Timed out or message closed.
[1503345617][WRP-C][Debug]: unpacking encoded data
[1503345617][WRP-C][Debug]: unpack_ret:2
[1503345617][WRP-C][Debug]: Binary payload {"message": "Hello Parodus"}
[1503345617][WRP-C][Debug]: spans is nil
[1503345617][hello-parodus][Debug]:     rtn = 0
[1503345617][hello-parodus][Info]: Got something from parodus.
[1503345617][hello-parodus][Info]: Request message type payload = {"message": "Hello Parodus"}
[1503345617][hello-parodus][Info]: Notification payload {"device_id":"mac:PCApplication","iot":"response"}
[1503345617][hello-parodus][Debug]: source: mac:123456789012/iot
[1503345617][hello-parodus][Debug]: destination: dns:scytale.webpa.comcast.net/iot
[1503345617][hello-parodus][Debug]: content_type is application/json
[1503345617][WRP-C][Debug]: ***   Start of Msgpack Encoding  ***
[1503345617][hello-parodus][Info]: Notification successfully sent to parodus
[1503345617][hello-parodus][Debug]: sendStatus is 0
```
3. The response to curl should be something similar to the following:-
```HTTP/1.1 200 OK
Content-Type: application/json
Date: Sat, 19 Aug 2017 01:57:47 GMT
X-Webpa-Device-Id: mac:123456789012
X-Webpa-Transaction-Id: f4ca37f0-8481-11e7-8df1-fa163e4d5327
Content-Length: 50

{"device_id":"mac:PCApplication","iot":"response”}
```
