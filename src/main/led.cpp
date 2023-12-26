// LED Control
// Uses single LED for error/status.
//========================================================================================
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"

#if defined(DPIN_LED)
#if defined(LED_ACTIVE_LOW) && LED_ACTIVE_LOW
#define LED_ON    LOW
#define LED_OFF   HIGH
#else // default is active high
#define LED_ON    HIGH
#define LED_OFF   LOW
#endif
#define LED_TURN_ON   digitalWrite(DPIN_LED, LED_ON)
#define LED_TURN_OFF  digitalWrite(DPIN_LED, LED_OFF)
#endif

void SetupLED(void)
{
  #if defined(DPIN_LED)
  pinMode(DPIN_LED, OUTPUT);
  #endif
  #if defined(DPIN_LED)
  LED_TURN_ON;
  #endif
}

void BlinkStatusLED(uint16_t slow, uint16_t fast)
{
  #if defined(DPIN_LED)
  LED_TURN_OFF;
  delay(250);

  if (slow)
  {
    for (uint16_t i = 0; i < slow; ++i)
    {
      LED_TURN_ON;
      delay(750);
      LED_TURN_OFF;
      delay(250);
    }

    if (fast) delay(250);
  }

  if (fast)
  {
    for (uint16_t i = 0; i < fast; ++i)
    {
      LED_TURN_ON;
      delay(150);
      LED_TURN_OFF;
      delay(250);
    }
  }

  delay(250);
  #endif
}

//========================================================================================
