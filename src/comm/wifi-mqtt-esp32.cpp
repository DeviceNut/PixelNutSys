// WiFi Communications using MQTT
/*
Copyright (c) 2022, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file

#include "main.h"

#if defined(ESP32) && WIFI_MQTT

#include "main/flash.h"
#include "wifi-mqtt-defs.h"

#include <ArduinoOTA.h>

class WiFiMqtt_Esp32 : public WiFiMqtt
{
protected:

  WiFiClient wifiClient;

  bool ConnectWiFi(int msecs); // attempts to connect to WiFi

  void ConnectOTA(void); // check for OTA code updates
};

WiFiMqtt_Esp32 wifiMqtt;
CustomCode *pCustomCode = &wifiMqtt;

bool WiFiMqtt_Esp32::ConnectWiFi(int msecs)
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

void WiFiMqtt_Esp32::ConnectOTA(void)
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

void WiFiMqtt::loop(void)
{
  ArduinoOTA.handle();

  if (!CheckConnections(false))
    BlinkStatusLED(1, 0);

  else mqttClient.loop();
}

#endif // WIFI_MQTT
