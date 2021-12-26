// PixelNutApp Serial Communications
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file

#include "main.h"
#include "flash.h"

#if COM_SERIAL

#define MAXLEN_INPUTSTR 100

class ComSerial : public CustomCode
{
public:

  #if EEPROM_FORMAT
  void flash(void) { setName(char*)(DEFAULT_DEVICE_NAME); }
  #endif

  void setup(void);
  void loop(void);

  void setName(char *name);
  void sendReply(char *instr);

private:

  char deviceName[MAXLEN_DEVICE_NAME + 1];
  char inputStr[MAXLEN_INPUTSTR + 1];
  int inputPos = 0;
};

ComSerial comSerial;
CustomCode *pCustomCode = &comSerial;

void ComSerial::sendReply(char *instr)
{
  Serial.println(instr);
}

void ComSerial::setName(char *name)
{
  FlashSetDevName(name);
  strcpy(deviceName, name);
}

void ComSerial::setup(void)
{
  FlashGetDevName(deviceName);

  DBGOUT(("---------------------------------------"));
  DBGOUT(("Serial Device: \"%s\"", deviceName));
  DBGOUT(("---------------------------------------"));
}

void ComSerial::loop(void)
{
  while (Serial.available() > 0)
  {
    char ch = Serial.read();
    if (ch == '\n')
    {
      inputStr[inputPos] = 0;
      ExecAppCmd(inputStr);
      inputPos = 0;
    }
    else if (ch != '\r')
    {
      inputStr[inputPos++] = ch;
      if (inputPos >= MAXLEN_INPUTSTR)
      {
        DBGOUT(("Serial command too long"));
        ErrorHandler(3, 3, false);
      inputPos = 0;
      }
    }
  }
}

#endif // COM_SERIAL
