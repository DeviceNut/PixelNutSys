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

#include "MQTT.h" // specific to Photon

SYSTEM_MODE(MANUAL); // prevent connecting to the Particle Cloud

extern void CallbackMqtt(char* topic, byte* message, unsigned int msglen);
static MQTT mqttClient(MQTT_BROKER_IPADDR, MQTT_BROKER_PORT, MAXLEN_PATSTR+100, CallbackMqtt);

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
  bool ConnectWiFi(void);               // attempts to connect to WiFi
  bool ConnectMqtt(void);               // attempts to connect to Mqtt

  void MakeHostName(void);
  void MakeMqttStrs(void);
};

#define WIFI_TEST(w)  (w.ready())
#define MQTT_TEST(m)  (m.isConnected())

#include "wifi-mqtt-code.h"

bool WiFiMqtt::ConnectWiFi(void)
{
  DBGOUT(("WiFi connecting..."));
  WiFi.connect();

  DBGOUT(("WiFi set credentials..."));
  WiFi.setHostname(hostName);
  WiFi.clearCredentials();
  WiFi.setCredentials(WIFI_CREDS_SSID, WIFI_CREDS_PASS);

  uint32_t tout = millis() + MSECS_WAIT_WIFI;
  while (millis() < tout)
  {
    DBGOUT(("WiFi test ready..."));
    if (WIFI_TEST(WiFi))
    {
      strcpy(localIP, WiFi.localIP().toString().c_str());
      MakeMqttStrs(); // uses deviceName and localIP

      DBGOUT(("WiFi ready at: %s", localIP));
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

  DBGOUT(("Mqtt Connect Failed"));
  BlinkStatusLED(1, 0);
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

  CheckConnections(true); // initial connection attempt

  DBGOUT(("---------------------------------------"));
}

void WiFiMqtt::loop(void)
{
  if (CheckConnections(false))
    mqttClient.loop();
}

#endif // WIFI_MQTT
