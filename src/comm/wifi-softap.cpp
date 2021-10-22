// PixelNutApp WiFi Communications using SoftAP
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (BSD License)
See license.txt for the terms of this license.
*/

#include "main.h"
#include "flash.h"

#if WIFI_SOFTAP

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

extern void CheckExecCmd(char *instr); // defined in main.h

class WiFiSoftAp : public CustomCode
{
public:

  #if EEPROM_FORMAT
  void flash(void) { setName(DEFAULT_DEVICE_NAME); }
  #endif

  void setup(void);
  void loop(void);

  void setName(char *name);
  void sendReply(char *instr);

private:

  const String strPutCommand = "POST /command HTTP/";
  const String strContentLength = "Content-Length: ";

  WiFiServer wifiServer = WiFiServer();

  IPAddress ipAddr = IPAddress(192, 168, 0, 1);

  char replyString[1000]; // long enough for maximum ?, ?S, ?P output strings

  void ServerConnection(void);
};

WiFiSoftAp wifiSAP;
CustomCode *pCustomCode = &wifiSAP;

void WiFiSoftAp::setup(void)
{
  char devname[MAXLEN_DEVICE_NAME + PREFIX_LEN_DEVNAME + 1];
  strcpy(devname, PREFIX_DEVICE_NAME);
  FlashGetDevName(devname + PREFIX_LEN_DEVNAME);

  DBGOUT(("---------------------------------------"));

  DBGOUT((F("Setting up SoftAP: %s..."), devname));

  WiFi.softAP(devname, NULL);
  delay(100); // do this or crashes when client connects !!
  WiFi.softAPConfig(ipAddr, ipAddr, IPAddress(255, 255, 255, 0));
  IPAddress myIP = WiFi.softAPIP();
  DBGOUT(("  IPaddr=%s", myIP.toString().c_str()));
  WiFi.softAPsetHostname("DeviceNut"); // FIXME
  DBGOUT(("  HostName=%s", WiFi.softAPgetHostname()));

  wifiServer.begin();

  DBGOUT(("---------------------------------------"));
}

void WiFiSoftAp::setName(char *name)
{
  FlashSetDevName(name);
}

void WiFiSoftAp::sendReply(char *instr)
{
  DBGOUT(("SoftAP TX: \"%s\"", instr));
  strcat(replyString, instr);
  strcat(replyString, "\r\n");
}

void WiFiSoftAp::loop(void)
{
  WiFiClient client = wifiServer.available();
  if (client)
  {
    String line = "";
    bool foundcmd = false;
    bool dobody = false;
    int bodylen = 0;
    int newlines = 0;

    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        //Serial.write(c);

        if (c == '\n')
        {
          if (++newlines >= 2) dobody = true;

          else if (!dobody)
          {
            if (line.startsWith(strPutCommand))
              foundcmd = true;

            else if (line.startsWith(strContentLength))
              bodylen = line.substring(strContentLength.length()).toInt();
          }
          else if (foundcmd)
          {
            char *instr = line.c_str();
            replyString[0] = 0;

            DBGOUT(("SoftAP RX: \"%s\"", instr));
            ExecAppCmd(instr);
          }
          else break;

          line = "";
        }
        else if (c != '\r')
        {
          line += c;
          newlines = 0;          
        }

        if (dobody && (bodylen-- <= 0))
          break;
      }
    }

    if (client.connected())
    {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/html");
      client.println();

      if (!foundcmd)
      {
        client.println("<!doctype html>");
        client.println("<html lang=en>");
        client.println("<head><title>DeviceNut</title></head>");
        client.println("<body><br><h3>Use PixelNutController App</h3></body>");
        client.println("</html>");
      } 
      else client.print(replyString);
    }

    client.stop();
  }
}

#endif // WIFI_SOFTAP
