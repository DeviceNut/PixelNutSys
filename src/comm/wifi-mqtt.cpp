// WiFi Communications using MQTT
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

#define DEBUG_OUTPUT 0 // 1 enables debugging this file

#include "main.h"
#include "main/flash.h"

#if WIFI_MQTT

extern void CallbackMqtt(char* topic, byte* message, unsigned int msglen);

#if defined(ESP32)
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#define WIFI_TEST(w)  (w.status() == WL_CONNECTED)
#define MQTT_TEST(m)  (m.connected())
#endif

#define MQTT_TOPIC_NOTIFY     "PixelNut/Notify"
#define MQTT_TOPIC_COMMAND    "PixelNut/Cmd/" // + name
#define MQTT_TOPIC_REPLY      "PixelNut/Reply"

#define STR_CONNECT_SEPARATOR ","
#define STRLEN_SEPARATOR      1

#define MAXLEN_DEVICE_IPSTR   15    // aaa.bbb.ccc.ddd
#define MSECS_CONNECT_PUB     1500  // msecs between MQtt publishes
#define MSECS_CONNECT_RETRY   1500  // msecs between connection retries

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

private:

  #if defined(ESP32)
  WiFiClient wifiClient;
  PubSubClient mqttClient;
	void ConnectOTA(void); // over-the-air code update
  #endif

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
  bool ConnectWiFi(int msecs);          // attempts to connect to WiFi
  bool ConnectMqtt(void);               // attempts to connect to Mqtt

  void MakeHostName(void);
  void MakeMqttStrs(void);
};

WiFiMqtt wifiMQTT;
CustomCode *pCustomCode = &wifiMQTT;

bool WiFiMqtt::CheckConnections(bool firstime)
{
  int msecs = MSECS_WAIT_WIFI;

  if (!firstime)
  {
    if (msecsRetryNotify <= millis())
    {
      haveWiFi = WIFI_TEST(WiFi);
      if (!haveWiFi)
      {
        mqttClient.disconnect();
        haveMqtt = false;
      }
      else haveMqtt = MQTT_TEST(mqttClient);
    }
    else return (haveWiFi && haveMqtt);

    msecs = MSECS_CONNECT_RETRY; // don't hold up loop when retrying
  }

  if (!haveWiFi && ConnectWiFi(msecs))
    haveWiFi = true;

  haveMqtt = haveWiFi && ConnectMqtt();

  msecsRetryNotify = millis() + MSECS_CONNECT_PUB;

  return (haveWiFi && haveMqtt);
}

#if defined(ESP32)
bool WiFiMqtt::ConnectWiFi(int msecs)
{
  DBGOUT(("Wifi checking status..."));

  uint32_t tout = millis() + msecs;
  while (millis() < tout)
  {
    if (WIFI_TEST(WiFi))
    {
      strcpy(localIP, WiFi.localIP().toString().c_str());
      MakeMqttStrs(); // uses deviceName and localIP
      DBGOUT(("WiFi ready at: %s", localIP));

      DBGOUT(("OTA connect..."));
      ConnectOTA();

      DBGOUT(("Mqtt client setup..."));
      mqttClient.setClient(wifiClient);
      mqttClient.setServer(MQTT_BROKER_IPADDR, MQTT_BROKER_PORT);
      mqttClient.setCallback(CallbackMqtt);

      return true;
    }

    BlinkStatusLED(1, 0);
  }

  DBGOUT(("WiFi connect failed!"));
  return false;
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
      Serial.println("OTA: Start updating " + type);
    })
    .onEnd([]()
    {
      Serial.println("\nOTA: End");
    })
    .onProgress([](unsigned int progress, unsigned int total)
    {
      Serial.printf("OTA: Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error)
    {
      Serial.printf("OTA: Error[%u]: ", error);

           if (error == OTA_AUTH_ERROR)     Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR)    Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR)  Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)  Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR)      Serial.println("End Failed");
    });

  ArduinoOTA.begin();
}
#endif

bool WiFiMqtt::ConnectMqtt(void)
{
  if (!MQTT_TEST(mqttClient))
  {
    DBGOUT(("Mqtt connecting..."));

    if (mqttClient.connect(deviceName))
    {
      DBGOUT(("Mqtt subscribe: %s", devnameTopic));
      mqttClient.subscribe(devnameTopic);
    }
  }

  if (MQTT_TEST(mqttClient))
  {
    mqttClient.publish(MQTT_TOPIC_NOTIFY, notifyStr);
    return true;
  }

  DBGOUT(("Mqtt connect failed!"));
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

  strcpy(notifyStr, deviceName);
  strcat(notifyStr, STR_CONNECT_SEPARATOR);
  strcat(notifyStr, localIP);
}

void WiFiMqtt::sendReply(char *instr)
{
  if (haveMqtt)
  {
    DBGOUT(("Mqtt TX: \"%s\"", instr));
    char *rstr = replyStr;
    sprintf(rstr, "%s\n%s", deviceName, instr);
    mqttClient.publish(MQTT_TOPIC_REPLY, replyStr);
  }
}

void WiFiMqtt::setName(char *name)
{
  #if !EEPROM_FORMAT
  if (haveMqtt)
  {
    DBGOUT(("Unsubscribe to: %s", devnameTopic));
    mqttClient.unsubscribe(devnameTopic);

    DBGOUT(("Disconnect from Mqtt..."));
    mqttClient.disconnect();
  }
  #endif

  FlashSetDevName(name);
  strcpy(deviceName, name);

  #if !EEPROM_FORMAT
  // re-connect with new name next loop
  MakeMqttStrs();
  DBGOUT(("Mqtt Device: %s", deviceName));
  msecsRetryNotify = 0;
  #endif
}

#if defined(ESP32)
void WiFiMqtt::setup(void)
{
  FlashGetDevName(deviceName);
  MakeHostName(); // uses deviceName

  DBGOUT(("---------------------------------------"));
  DBGOUT(("WiFi: %s as %s", WIFI_CREDS_SSID, hostName));

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_CREDS_SSID, WIFI_CREDS_PASS);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostName);

  DBGOUT(("Mqtt Device: %s", deviceName));
  DBGOUT(("Mqtt Broker: %s:%d", MQTT_BROKER_IPADDR, MQTT_BROKER_PORT));
  DBGOUT(("  MaxBufSize=%d", MQTT_MAX_PACKET_SIZE));
  DBGOUT(("  KeepAliveSecs=%d", MQTT_KEEPALIVE));

  if (!CheckConnections(true)) // initial connection attempt
    ErrorHandler(3, 1, false);

  DBGOUT(("---------------------------------------"));
}
#endif

void WiFiMqtt::loop(void)
{
	#if defined(ESP32)
  ArduinoOTA.handle();
	#endif

  if (!CheckConnections(false))
    BlinkStatusLED(1, 0);

  else mqttClient.loop();
}

#endif // WIFI_MQTT
