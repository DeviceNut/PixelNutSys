// WiFi Communications using MQTT Common Defines
/*
Copyright (c) 2022, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "mydevices.h"

/*****************************************************************************************
 Protocol used with MQTT:

 1) This Client has the Wifi credentials and the Broker's IP hardcoded in "mydevices.h".

 2) Client sends to Broker (topic="PixelNutNotify"): <DevName>, <IPaddr>
    IPaddr: local ip address (e.g. 192.168.1.122)
    DevName: Friendly name of this device (e.g. "My Device")
    This is sent periodically to maintain a connection.

 3) Broker sends command to Client (topic is <DevName>): <cmdstr>
    <cmdstr> is a PixelNut command string.

 4) If command starts with "?" then client will reply (topic="PixelNutReply"): <reply>

*****************************************************************************************/

#define MQTT_TOPIC_NOTIFY     "PixelNut/Notify"
#define MQTT_TOPIC_COMMAND    "PixelNut/Cmd/" // + name
#define MQTT_TOPIC_REPLY      "PixelNut/Reply"

#define STR_CONNECT_SEPARATOR ","
#define STRLEN_SEPARATOR      1

#define MAXLEN_DEVICE_IPSTR   15    // aaa.bbb.ccc.ddd
#define MSECS_CONNECT_PUB     1000  // msecs between connection retries and MQtt publishes
