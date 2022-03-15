// WiFi Communications using MQTT Common Code
/*
Copyright (c) 2022, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

WiFiMqtt wifiMQTT;
CustomCode *pCustomCode = &wifiMQTT;

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
    char *rstr = replyStr;
    sprintf(rstr, "%s\n%s", deviceName, instr);
    mqttClient.publish(MQTT_TOPIC_REPLY, replyStr);
  }
}
