// PixelNutApp Serial Communications
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file

#include "main.h"
#include "main/flash.h"

#if COM_SERIAL

#define MAXLEN_INPUTSTR 100

class ComSerial : public CustomCode
{
public:

  void setup(void);
  void loop(void);

  void setName(char *name);
  void sendReply(char *instr);

private:

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
}

void ComSerial::setup(void)
{
  #if DEBUG_OUTPUT
  char deviceName[MAXLEN_DEVICE_NAME + 1];
  FlashGetDevName(deviceName);

  DBGOUT(("---------------------------------------"));
  DBGOUT(("Serial Device: \"%s\"", deviceName));
  DBGOUT(("---------------------------------------"));
  #endif
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
      if (inputPos >= MAXLEN_INPUTSTR)
      {
        DBGOUT(("Serial command > %d", MAXLEN_INPUTSTR));
        ErrorHandler(3, 1, false);
        inputPos = 0;
      }
      else inputStr[inputPos++] = ch;
    }
  }
}

#endif // COM_SERIAL
