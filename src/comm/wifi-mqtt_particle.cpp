// WiFi Communications using MQTT for Particle Photon
/*
Copyright (c) 2022, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file

#include "main.h"
#include "main/flash.h"

#if defined(PARTICLE) && WIFI_MQTT

#include "wifi-mqtt-defs.h"

SYSTEM_MODE(MANUAL); // prevent connecting to the Particle Cloud

class WiFiMqtt_Particle : public WiFiMqtt
{
protected:

  bool ConnectWiFi(int msecs); // attempts to connect to WiFi

  void ConnectOTA(void); // check for OTA code updates
};

WiFiMqtt_Particle wifiMqtt;
CustomCode *pCustomCode = &wifiMqtt;

bool WiFiMqtt_Particle::ConnectWiFi(int msecs)
{
  DBGOUT(("WiFi connecting..."));
  WiFi.connect();

  DBGOUT(("WiFi set credentials..."));
  WiFi.setHostname(hostName);
  WiFi.clearCredentials();
  WiFi.setCredentials(WIFI_CREDS_SSID, WIFI_CREDS_PASS);

  uint32_t tout = millis() + msecs;
  while (millis() < tout)
  {
    DBGOUT(("WiFi test ready..."));
    if (WIFI_TEST(WiFi)) // does not return right away if fails
    {
      strcpy(localIP, WiFi.localIP().toString().c_str());
      MakeMqttStrs(); // uses deviceName and localIP

      DBGOUT(("WiFi ready at: %s", localIP));
      return true;
    }

    BlinkStatusLED(1, 0);
  }

  DBGOUT(("WiFi connect failed!"));
  return false;
}

void WiFiMqtt::setup(void)
{
  FlashGetDevName(deviceName);
  MakeHostName(); // uses deviceName

  DBGOUT(("---------------------------------------"));
  DBGOUT(("WiFi: %s as %s", WIFI_CREDS_SSID, hostName));
  DBGOUT(("Mqtt Device: %s", deviceName));
  DBGOUT(("Mqtt Broker: %s:%d", MQTT_BROKER_IPADDR, MQTT_BROKER_PORT));

  if (!CheckConnections(true)) // initial connection attempt
    ErrorHandler(3, 1, false);

  DBGOUT(("---------------------------------------"));
}

void WiFiMqtt::loop(void)
{
  if (!CheckConnections(false))
    BlinkStatusLED(1, 0);

  else mqttClient.loop();
}

#endif // WIFI_MQTT
