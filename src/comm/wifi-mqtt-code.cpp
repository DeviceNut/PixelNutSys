// WiFi Communications using MQTT Common Code
/*
Copyright (c) 2022, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file

#include "main.h"

#if WIFI_MQTT

#include "main/flash.h"
#include "wifi-mqtt-defs.h"

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

  for (size_t i = 0; i < strlen(deviceName); ++i)
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

#endif // WIFI_MQTT
