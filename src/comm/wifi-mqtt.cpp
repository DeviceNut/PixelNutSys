// PixelNutApp WiFi Communications using MQTT
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 1 // 1 enables debugging this file (must also set in main.h)

#include "main.h"
#include "flash.h"

#if WIFI_MQTT

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#include "mycredentials.h"
/* Wifi SSID/Password and MQTT Broker in the following format:
#define WIFI_CREDS_SSID "SSID"
#define WIFI_CREDS_PASS "PASSWORD"
#define MQTT_BROKER_IPADDR "192.168.1.4"
#define MQTT_BROKER_PORT 1883
*/

/*****************************************************************************************
 Protocol used with MQTT:

 1) This Client has the Wifi credentials and the Broker's IP hardcoded.

 2) Client sends to Broker (topic="PixelNutNotify"): <DevName>, <IPaddr>
    IPaddr: local ip address (e.g. 192.168.1.122)
    DevName: Friendly name of this device (e.g. "My Device")
    This is sent periodically to maintain a connection.

 3) Broker sends command to Client (topic is <DevName>): <cmdstr>
    <cmdstr> is a PixelNut command string.

 4) If command starts with "?" then client will reply (topic="PixelNutReply"): <reply>
    <reply> is one or more lines of text with information, depending on the command.

 4) If command is "?P" then the reply is a list of custom patterns stored in the device.

*****************************************************************************************/

#define MQTT_TOPIC_NOTIFY     "PixelNut/Notify"
#define MQTT_TOPIC_COMMAND    "PixelNut/Cmd/" // + name
#define MQTT_TOPIC_REPLY      "PixelNut/Reply"

#define STR_CONNECT_SEPARATOR ","
#define STRLEN_SEPARATOR      1

#define MAXLEN_DEVICE_IPSTR   15 // aaa.bbb.ccc.ddd
#define MSECS_CONNECT_PUB     2000 // msecs between connect publishes

class WiFiMqtt : public CustomCode
{
public:

  #if EEPROM_FORMAT
  void flash(void) { setName(DEFAULT_DEVICE_NAME); }
  #endif

  void setup(void);
  void loop(void);

  void setName(char *name);
  void sendReply(char *instr);

  PubSubClient mqttClient;

  // creates the topic name for sending cmds
  // needs to be public to be used in callback
  char deviceName[MAXLEN_DEVICE_NAME + 1];
  char hostName[PREFIX_LEN_DEVNAME + MAXLEN_DEVICE_NAME + 1];
  char replyStr[1000]; // long enough for all segments

private:

  WiFiClient wifiClient;

  char localIP[MAXLEN_DEVICE_IPSTR];  // local IP address

  // topic to subscribe to, with device name
  char devnameTopic[sizeof(MQTT_TOPIC_COMMAND) + MAXLEN_DEVICE_NAME + 1];
  // string sent to the MQTT_TOPIC_NOTIFY topic
  char connectStr[MAXLEN_DEVICE_IPSTR + STRLEN_SEPARATOR + MAXLEN_DEVICE_NAME + 1];
  uint32_t nextConnectTime = 0; // next time to send notify string

  void ConnectWiFi(void);   // waits for connection to WiFi
  bool ConnectMqtt(void);   // returns True if now connected

  void MakeHostName(void);
  void MakeMqttStrs(void);
};

WiFiMqtt wifiMQTT;
CustomCode *pCustomCode = &wifiMQTT;

void WiFiMqtt::ConnectWiFi()
{
  DBGOUT(("Connect to WiFi..."));
  WiFi.begin(WIFI_CREDS_SSID, WIFI_CREDS_PASS);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostName); // FIXME: make unique?

  while (WiFi.status() != WL_CONNECTED)
    BlinkStatusLED(1, 0);
}

bool WiFiMqtt::ConnectMqtt(void)
{
  if (nextConnectTime >= millis())
    return mqttClient.connected();

  nextConnectTime = millis() + MSECS_CONNECT_PUB;

  if (!mqttClient.connected())
  {
    DBGOUT(("Connecting to Mqtt..."));

    // this crash/reboots if no broker found FIXME
    if (mqttClient.connect(deviceName))
    {
      DBGOUT(("Subscribe to: %s", devnameTopic));
      mqttClient.subscribe(devnameTopic);
    }
  }

  if (mqttClient.connected())
  {
    mqttClient.publish(MQTT_TOPIC_NOTIFY, connectStr, false);
    return true;
  }

  DBGOUT(("Mqtt State: %s", mqttClient.state()));
  BlinkStatusLED(0, 1);
  return false;
}

void CallbackMqtt(char* topic, byte* message, unsigned int msglen)
{
  //DBGOUT(("Callback for topic: %s", topic));

  // ignore 'topic' as there is only one
  if (msglen <= 0) return;

  while (*message == ' ')
  {
    ++message; // skip spaces
    --msglen;
    if (msglen <= 0) return;
  }

  if (msglen <= MAXLEN_PATSTR) // msglen doesn't include terminator
  {
    char instr[MAXLEN_PATSTR+1];
    strncpy(instr, (char*)message, msglen);
    instr[msglen] = 0;

    //DBGOUT(("Mqtt RX: \"%s\"", instr));
    ExecAppCmd(instr);
  }
  else { DBGOUT(("MQTT message too long: %d bytes", msglen)); }
}

void WiFiMqtt::MakeHostName(void)
{
  strcpy(hostName, PREFIX_DEVICE_NAME);
  char *str = hostName + strlen(hostName);

  for (int i = 0; i < strlen(deviceName); ++i)
  {
    char ch = deviceName[i];
    if (ch != ' ') *str++ = ch;
  }
  *str = 0;
}

void WiFiMqtt::MakeMqttStrs(void)
{
  strcpy(devnameTopic, MQTT_TOPIC_COMMAND);
  strcat(devnameTopic, deviceName);

  strcpy(connectStr, deviceName);
  strcat(connectStr, STR_CONNECT_SEPARATOR);
  strcat(connectStr, localIP);
}

void WiFiMqtt::sendReply(char *instr)
{
  char *rstr = wifiMQTT.replyStr;
  sprintf(rstr, "%s\n%s", wifiMQTT.deviceName, instr);

  DBGOUT(("Mqtt TX: \"%s\"", wifiMQTT.replyStr));
  wifiMQTT.mqttClient.publish(MQTT_TOPIC_REPLY, wifiMQTT.replyStr);
}

void WiFiMqtt::setName(char *name)
{
  DBGOUT(("Unsubscribe to: %s", devnameTopic));
  mqttClient.subscribe(devnameTopic);

  DBGOUT(("Disconnect from Mqtt..."));
  mqttClient.disconnect();

  FlashSetDevName(name);
  strcpy(deviceName, name);

  // re-connect with new name next loop
  MakeMqttStrs();
  nextConnectTime = 0;
}

void WiFiMqtt::setup(void)
{
  FlashGetDevName(deviceName);
  MakeHostName();

  DBGOUT(("---------------------------------------"));
  DBGOUT((F("Setting up WiFi/Mqtt...")));

  ConnectWiFi();
  mqttClient.setClient(wifiClient);
  mqttClient.setServer(MQTT_BROKER_IPADDR, MQTT_BROKER_PORT);
  mqttClient.setCallback(CallbackMqtt);

  strcpy(localIP, WiFi.localIP().toString().c_str());
  MakeMqttStrs();

  DBGOUT(("Mqtt Device: \"%s\"", deviceName));
  DBGOUT(("  LocalIP=%s", localIP));
  DBGOUT(("  Hostname=%s", WiFi.getHostname()));
  DBGOUT(("  Broker=%s:%d", MQTT_BROKER_IPADDR, MQTT_BROKER_PORT));
  DBGOUT(("  MaxBufSize=%d", MQTT_MAX_PACKET_SIZE));
  DBGOUT(("  KeepAliveSecs=%d", MQTT_KEEPALIVE));
  DBGOUT(("---------------------------------------"));

  ConnectMqtt();
}

void WiFiMqtt::loop(void)
{
    if (ConnectMqtt()) mqttClient.loop();
}

#endif // WIFI_MQTT
