// WiFi Communications using MQTT
/*
Copyright (c) 2022, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file

#include "main.h"
#include "main/flash.h"

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

  WiFiClient wifiClient;
  PubSubClient mqttClient;

  char localIP[MAXLEN_DEVICE_IPSTR];  // local IP address

  // topic to subscribe to, with device name
  char devnameTopic[sizeof(MQTT_TOPIC_COMMAND) + MAXLEN_DEVICE_NAME + 1];
  // string sent to the MQTT_TOPIC_NOTIFY topic
  char notifyStr[MAXLEN_DEVICE_IPSTR + STRLEN_SEPARATOR + MAXLEN_DEVICE_NAME + 1];

  bool haveWiFi = false;
  bool haveMqtt = false;
  uint32_t msecsRetryNotify = 0; // next time to send notify string

  // creates the topic name for sending cmds
  // needs to be public to be used in callback
  char deviceName[MAXLEN_DEVICE_NAME + 1];
  char hostName[strlen(PREFIX_DEVICE_NAME) + MAXLEN_DEVICE_NAME + 1];
  char replyStr[1000]; // long enough for all segments

  bool CheckConnections(bool firstime); // returns true if both WiFi/Mqtt connected
  bool ConnectWiFi(void);               // attempts to connect to WiFi
  bool ConnectMqtt(void);               // attempts to connect to Mqtt
  void ConnectOTA(void);                // connects to OTA if present

  void MakeHostName(void);
  void MakeMqttStrs(void);
};

#define WIFI_TEST(w)  (w.status() == WL_CONNECTED)
#define MQTT_TEST(m)  (m.connected())

#include "wifi-mqtt-code.h"

bool WiFiMqtt::ConnectWiFi(void)
{
  uint32_t tout = millis() + MSECS_WAIT_WIFI;
  while (millis() < tout)
  {
    DBGOUT(("Wifi check status..."));

    if (WiFi.status() == WL_CONNECTED)
    {
      strcpy(localIP, WiFi.localIP().toString().c_str());
      MakeMqttStrs(); // uses deviceName and localIP

      DBGOUT(("WiFi ready at: %s", localIP));
      DBGOUT(("  Hostname=%s", WiFi.getHostname()));
      return true;
    }

    BlinkStatusLED(1, 0);
  }

  DBGOUT(("WiFi Connect Failed"));
  BlinkStatusLED(1, 0);
  return false;
}

bool WiFiMqtt::ConnectMqtt(void)
{
  if (!MQTT_TEST(mqttClient))
  {
    DBGOUT(("Mqtt connecting..."));

    // this crash/reboots if no broker found FIXME??
    if (mqttClient.connect(deviceName))
    {
      mqttClient.setClient(wifiClient);
      mqttClient.setServer(MQTT_BROKER_IPADDR, MQTT_BROKER_PORT);
      mqttClient.setCallback(CallbackMqtt);

      DBGOUT(("Mqtt subscribe: %s", devnameTopic));
      mqttClient.subscribe(devnameTopic);
    }
  }

  if (MQTT_TEST(mqttClient))
  {
    mqttClient.publish(MQTT_TOPIC_NOTIFY, notifyStr, false);
    return true;
  }

  DBGOUT(("Mqtt Connect Failed: state=%s", mqttClient.state()));
  BlinkStatusLED(1, 0);
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

           if (error == OTA_AUTH_ERROR)     Serial.println("OTA: Auth Failed");
      else if (error == OTA_BEGIN_ERROR)    Serial.println("OTA: Begin Failed");
      else if (error == OTA_CONNECT_ERROR)  Serial.println("OTA: Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)  Serial.println("OTA: Receive Failed");
      else if (error == OTA_END_ERROR)      Serial.println("OTA: End Failed");
    });

  ArduinoOTA.begin();
}

void WiFiMqtt::setup(void)
{
  FlashGetDevName(deviceName);
  MakeHostName(); // uses deviceName

  ConnectOTA();

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

  CheckConnections(true); // initial connection attempt

  DBGOUT(("---------------------------------------"));
}

void WiFiMqtt::loop(void)
{
  ArduinoOTA.handle();

  if (CheckConnections(false))
    mqttClient.loop();
}

#endif // WIFI_MQTT
