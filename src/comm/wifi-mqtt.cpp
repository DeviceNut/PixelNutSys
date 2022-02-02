// PixelNutApp WiFi Communications using MQTT
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 1 // 1 enables debugging this file

#include "main.h"
#include "flash.h"

#if WIFI_MQTT

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

#include "mydevices.h"

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

*****************************************************************************************/

#define MQTT_TOPIC_NOTIFY     "PixelNut/Notify"
#define MQTT_TOPIC_COMMAND    "PixelNut/Cmd/" // + name
#define MQTT_TOPIC_REPLY      "PixelNut/Reply"

#define STR_CONNECT_SEPARATOR ","
#define STRLEN_SEPARATOR      1

#define MAXLEN_DEVICE_IPSTR   15    // aaa.bbb.ccc.ddd
#define MSECS_CONNECT_PUB     1000  // msecs between connect publishes

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

  PubSubClient mqttClient;

  // creates the topic name for sending cmds
  // needs to be public to be used in callback
  char deviceName[MAXLEN_DEVICE_NAME + 1];
  char hostName[strlen(PREFIX_DEVICE_NAME) + MAXLEN_DEVICE_NAME + 1];
  char replyStr[1000]; // long enough for all segments

private:

  bool haveConnection = false;
  WiFiClient wifiClient;

  char localIP[MAXLEN_DEVICE_IPSTR];  // local IP address

  // topic to subscribe to, with device name
  char devnameTopic[sizeof(MQTT_TOPIC_COMMAND) + MAXLEN_DEVICE_NAME + 1];
  // string sent to the MQTT_TOPIC_NOTIFY topic
  char connectStr[MAXLEN_DEVICE_IPSTR + STRLEN_SEPARATOR + MAXLEN_DEVICE_NAME + 1];
  uint32_t nextConnectTime = 0; // next time to send notify string

  void ConnectWiFi(void);   // waits for connection to WiFi
  void ConnectOTA(void);    // connects to OTA if present
  bool ConnectMqtt(void);   // returns True if now connected

  void MakeHostName(void);
  void MakeMqttStrs(void);
};

WiFiMqtt wifiMQTT;
CustomCode *pCustomCode = &wifiMQTT;

void WiFiMqtt::ConnectWiFi(void)
{
  DBGOUT(("Connect to WiFi: %s ...", WIFI_CREDS_SSID));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_CREDS_SSID, WIFI_CREDS_PASS);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostName); // FIXME: make unique?

  uint32_t tout = millis() + MSECS_WAIT_WIFI;
  while (tout <= millis())
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      haveConnection = true;
      break;
    }
    BlinkStatusLED(1, 0);
  }

  DBG( if (!haveConnection) DBGOUT(("Failed to find WiFi!")); )
}

void WiFiMqtt::ConnectOTA(void)
{
  ArduinoOTA
    .onStart([]()
    {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]()
    {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total)
    {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error)
    {
      Serial.printf("Error[%u]: ", error);

           if (error == OTA_AUTH_ERROR)     Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR)    Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR)  Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)  Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR)      Serial.println("End Failed");
    });

  ArduinoOTA.begin();
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
  BlinkStatusLED(1, 0);
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

    DBGOUT(("Mqtt RX: \"%s\"", instr));
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
  if (haveConnection)
  {
    DBGOUT(("Mqtt TX: \"%s\"", instr));
    char *rstr = wifiMQTT.replyStr;
    sprintf(rstr, "%s\n%s", wifiMQTT.deviceName, instr);
    wifiMQTT.mqttClient.publish(MQTT_TOPIC_REPLY, wifiMQTT.replyStr);
  }
}

void WiFiMqtt::setName(char *name)
{
  #if !EEPROM_FORMAT
  if (haveConnection)
  {
    DBGOUT(("Unsubscribe to: %s", devnameTopic));
    mqttClient.subscribe(devnameTopic);

    DBGOUT(("Disconnect from Mqtt..."));
    mqttClient.disconnect();
  }
  #endif

  FlashSetDevName(name);
  strcpy(deviceName, name);

  #if !EEPROM_FORMAT
  if (haveConnection)
  {
    // re-connect with new name next loop
    MakeMqttStrs();
    nextConnectTime = 0;
  }
  #endif
}

void WiFiMqtt::setup(void)
{
  FlashGetDevName(deviceName);
  MakeHostName();

  DBGOUT(("---------------------------------------"));
  DBGOUT((F("Setting up WiFi/Mqtt...")));

  ConnectWiFi();
  ConnectOTA();

  if (haveConnection)
  {
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
}

void WiFiMqtt::loop(void)
{
  ArduinoOTA.handle();

  if (haveConnection && ConnectMqtt())
    mqttClient.loop();
}

#endif // WIFI_MQTT
