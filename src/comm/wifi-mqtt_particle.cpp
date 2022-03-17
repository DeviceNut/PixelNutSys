// WiFi Communications using MQTT for Particle Photon
/*
Copyright (c) 2022, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 1 // 1 enables debugging this file

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

  bool haveConnection = false;

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
  bool ConnectMqtt(void);   // returns True if now connected

  void MakeHostName(void);
  void MakeMqttStrs(void);
};

#include "wifi-mqtt-code.h"

void WiFiMqtt::ConnectWiFi(void)
{
  DBGOUT(("Connect to WiFi: %s as %s ...", WIFI_CREDS_SSID, hostName));
  WiFi.setCredentials(WIFI_CREDS_SSID, WIFI_CREDS_PASS);
  WiFi.setHostname(hostName);
  WiFi.connect();

  uint32_t tout = millis() + MSECS_WAIT_WIFI;
  while (millis() < tout)
  {
    if (WiFi.ready())
    {
      haveConnection = true;
      break;
    }
    BlinkStatusLED(1, 0);
  }

  DBG( if (!haveConnection) DBGOUT(("Failed to find WiFi!")); )
}

bool WiFiMqtt::ConnectMqtt(void)
{
  if (nextConnectTime >= millis())
    return mqttClient.isConnected();

  nextConnectTime = millis() + MSECS_CONNECT_PUB;

  if (!mqttClient.isConnected())
  {
    DBGOUT(("Connecting to Mqtt as %s ...", deviceName));

    if (mqttClient.connect(deviceName))
    {
      DBGOUT(("Subscribe to: %s", devnameTopic));
      mqttClient.subscribe(devnameTopic);
    }
  }

  if (mqttClient.isConnected())
  {
    mqttClient.publish(MQTT_TOPIC_NOTIFY, connectStr);
    return true;
  }

  DBGOUT(("Mqtt not connected"));
  BlinkStatusLED(1, 0);
  return false;
}

void WiFiMqtt::setup(void)
{
  FlashGetDevName(deviceName);
  MakeHostName();

  DBGOUT(("---------------------------------------"));
  DBGOUT(("Setting up WiFi/Mqtt..."));

  ConnectWiFi();

  if (haveConnection)
  {
    strcpy(localIP, WiFi.localIP().toString().c_str());
    MakeMqttStrs();

    DBGOUT(("Mqtt Device: \"%s\"", deviceName));
    DBGOUT(("  LocalIP=%s", localIP));
    DBGOUT(("  Hostname=%s", WiFi.hostname()));
    DBGOUT(("  Broker=%s:%d", MQTT_BROKER_IPADDR, MQTT_BROKER_PORT));
    DBGOUT(("---------------------------------------"));

    ConnectMqtt();
  }
}

void WiFiMqtt::loop(void)
{
  if (haveConnection && ConnectMqtt())
    mqttClient.loop();
}

#endif // WIFI_MQTT
