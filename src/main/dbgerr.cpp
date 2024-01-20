// Debug and Error Handling Routines
/*
Copyright (c) 2024, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"

#if DEBUG_OUTPUT

// debug output string must be longer than pattern strings and any debug format string
#define MAXLEN_DBGSTR (MAXLEN_PATSTR + 70)
char dbgstr[MAXLEN_DBGSTR];   // holds debug output string

void MsgFormat(const char *fmtstr, ...)
{
  va_list va;
  va_start(va, fmtstr);
  vsnprintf(dbgstr, MAXLEN_DBGSTR, fmtstr, va);
  va_end(va);

  Serial.println(dbgstr);
}

void SetupDBG(void)
{
  Serial.begin(SERIAL_BAUD_RATE);

  #if !defined(ESP32)
  #if (MSECS_WAIT_SERIAL > 0)
  uint32_t tout = millis() + MSECS_WAIT_SERIAL;
  while (!Serial) // wait for serial monitor
  {
    BlinkStatusLED(0, 1);
    if (millis() > tout) break;
  }
  #endif
  #endif

  // wait a tad longer, then send sign-on message
  delay(10);
  DBGOUT((F(DEBUG_SIGNON)));
}

#else
void SetupDBG(void) {}
void MsgFormat(const char *fmtstr, ...) {}
#endif

// does not return if 'dostop' is true
void ErrorHandler(short slow, short fast, bool dostop)
{
  DBGOUT((F("Error code: 0x%02X"), ((slow << 4) | fast)));

  do BlinkStatusLED(slow, fast);
  while (dostop);
}
