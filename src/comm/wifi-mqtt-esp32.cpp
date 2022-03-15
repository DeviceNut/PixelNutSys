// WiFi Communications using MQTT
/*
Copyright (c) 2022, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file

#include "main.h"
#include "flash.h"

#if defined(ESP32) && WIFI_MQTT

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

#include "wifi-mqtt-defs.h"

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

  bool haveConnection = false;
  WiFiClient wifiClient;
  PubSubClient mqttClient;

  char localIP[MAXLEN_DEVICE_IPSTR];  // local IP address

  // topic to subscribe to, with device name
  char devnameTopic[sizeof(MQTT_TOPIC_COMMAND) + MAXLEN_DEVICE_NAME + 1];
  // string sent to the MQTT_TOPIC_NOTIFY topic
  char connectStr[MAXLEN_DEVICE_IPSTR + STRLEN_SEPARATOR + MAXLEN_DEVICE_NAME + 1];
  uint32_t nextConnectTime = 0; // next time to send notify string

  // creates the topic name for sending cmds
  // needs to be public to be used in callback
  char deviceName[MAXLEN_DEVICE_NAME + 1];
  char hostName[strlen(PREFIX_DEVICE_NAME) + MAXLEN_DEVICE_NAME + 1];
  char replyStr[1000]; // long enough for all segments

  void ConnectWiFi(void);   // waits for connection to WiFi
  void ConnectOTA(void);    // connects to OTA if present
  bool ConnectMqtt(void);   // returns True if now connected

  void MakeHostName(void);
  void MakeMqttStrs(void);
};

#include "wifi-mqtt-code.h"

void WiFiMqtt::ConnectWiFi(void)
{
  DBGOUT(("Connect to WiFi: %s ...", WIFI_CREDS_SSID));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_CREDS_SSID, WIFI_CREDS_PASS);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostName);

  uint32_t tout = millis() + MSECS_WAIT_WIFI;
  while (millis() < tout)
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

    // this crash/reboots if no broker found FIXME??
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

void WiFiMqtt::sendReply(char *instr)
{
  if (haveConnection)
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
  if (haveConnection)
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
