// Hardware Delay Control
// Sets global delay percent applied to all effects in the pixelnut engine.
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"
#include "UIDeviceButton.h"
#include "UIDeviceAnalog.h"

#if defined(DPIN_DELAY_BUTTON) || defined(APIN_DELAY_POT)

// engine support setting of global delay offset
static void SetDelayPercent(byte pcent = MAX_PERCENTAGE)
{
  DBGOUT((F("Controls: delay percent=%d%%"), pcent));
  pPixelNutEngine->setDelayPercent(pcent);
}

#endif

//========================================================================================
#if defined(DPIN_DELAY_BUTTON)

// allow repeating and long press, but not double-click
UIDeviceButton bc_delay(DPIN_DELAY_BUTTON, false, true, true);

static byte delay_pos = 2; // default setting
static int8_t delay_presets[] = { 10, 30, 50, 70, 90 };

static void SetNewDelay(void)
{
  if (delay_pos >= sizeof(delay_presets)/sizeof(delay_presets[0]))
    delay_pos = 0;

  SetDelayPercent( delay_presets[delay_pos] );
}

static void CheckDelay(void)
{
  if (bc_delay.CheckForChange() != UIDeviceButton::Retcode_NoChange)
  {
    ++delay_pos;
    SetNewDelay();
  }
}

static void SetupDelay(void)
{
  // can adjust button settings here...

  SetNewDelay();
}

//========================================================================================
// cannot use delay button AND a pot
#elif defined(APIN_DELAY_POT)

// we define this so that the animation gets faster when increasing the pot
#if defined(DELAY_POT_BACKWARDS) && DELAY_POT_BACKWARDS
UIDeviceAnalog pc_delay(APIN_DELAY_POT, 0, MAX_PERCENTAGE);
#else // default is not wired backwards
UIDeviceAnalog pc_delay(APIN_DELAY_POT, MAX_PERCENTAGE, 0);
#endif

static void CheckDelay(void)
{
  if (pc_delay.CheckForChange())
    SetDelayPercent( pc_delay.newValue );
}

static void SetupDelay(void)
{
  DBGOUT((F("Delay: range=%d-%d offset=%d msecs"), 0, MAX_PERCENTAGE));
  SetDelayPercent( pc_delay.newValue );
}

#endif
//========================================================================================

// initialize controls
void SetupDelayControls(void)
{
  #if defined(DPIN_DELAY_BUTTON) || defined(APIN_DELAY_POT)
  SetupDelay();
  #endif
}

// called every control loop
void CheckDelayControls(void)
{
  #if defined(DPIN_DELAY_BUTTON) || defined(APIN_DELAY_POT)
  CheckDelay();
  #endif
}

//========================================================================================
