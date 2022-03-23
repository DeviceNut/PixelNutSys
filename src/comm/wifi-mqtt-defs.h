// WiFi Communications using MQTT Common Defines
/*
Copyright (c) 2022, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

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
#define MSECS_CONNECT_PUB     1500  // msecs between MQtt publishes
#define MSECS_CONNECT_RETRY   1500  // msecs between connection retries

extern void CallbackMqtt(char* topic, byte* message, unsigned int msglen);

#if defined(PARTICLE)
#include "MQTT.h" // specific to Photon
static MQTT mqttClient(MQTT_BROKER_IPADDR, MQTT_BROKER_PORT, MAXLEN_PATSTR+100, CallbackMqtt);
#define WIFI_TEST(w)  (w.ready())
#define MQTT_TEST(m)  (m.isConnected())
#endif

#if defined(ESP32)
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#define WIFI_TEST(w)  (w.status() == WL_CONNECTED)
#define MQTT_TEST(m)  (m.connected())
#endif
class WiFiMqtt : public CustomCode
{
public:

  #if EEPROM_FORMAT
  void flash(void) { setName((char*)DEFAULT_DEVICE_NAME); }
  #endif

  void setup(void);
  void loop(void);

  void setName(char *name);
  void sendReply(char *instr);

protected:

 WiFiClient wifiClient;
 PubSubClient mqttClient;
  char localIP[MAXLEN_DEVICE_IPSTR];  // local IP address

  // topic to subscribe to, with device name
  char devnameTopic[sizeof(MQTT_TOPIC_COMMAND) + MAXLEN_DEVICE_NAME + 1];

  // string sent to the MQTT_TOPIC_NOTIFY topic
  char notifyStr[MAXLEN_DEVICE_IPSTR + STRLEN_SEPARATOR + MAXLEN_DEVICE_NAME + 1];

  bool haveWiFi = false;
  bool haveMqtt = false;
  uint32_t msecsRetryNotify = 0; // next time to retry connections or send notify string

  // creates the topic name for sending cmds
  // needs to be public to be used in callback
  char deviceName[MAXLEN_DEVICE_NAME + 1];
  char hostName[strlen(PREFIX_DEVICE_NAME) + MAXLEN_DEVICE_NAME + 1];
  char replyStr[1000]; // long enough for all segments

  bool CheckConnections(bool firstime); // returns true if both WiFi/Mqtt connected
  bool ConnectMqtt(void);               // attempts to connect to Mqtt

  virtual bool ConnectWiFi(int msecs)=0; // attempts to connect to WiFi

  void MakeHostName(void);
  void MakeMqttStrs(void);
};
