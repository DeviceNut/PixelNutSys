// Hardware Trigger/Force Controls
// Sets the trigger force and triggers from physical controls.
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"
#include "UIDeviceButton.h"
#include "UIDeviceAnalog.h"

#if defined(DPIN_TRIGGER_BUTTON)

// no double-click, repeating, or long press
UIDeviceButton bc_trigger(DPIN_TRIGGER_BUTTON);

#if defined(APIN_TRIGGER_POT)
// allow headroom on output value to insure can reach min/max
UIDeviceAnalog pc_trigger(APIN_TRIGGER_POT, -10, MAX_FORCE_VALUE+10);
#endif

static void CheckTriggerButton(void)
{
  #if defined(APIN_TRIGGER_POT)
  pc_trigger.CheckForChange();
  #endif

  if ( bc_trigger.CheckForChange() != UIDeviceButton::Retcode_NoChange)
  {
    byte force;

    #if defined(APIN_TRIGGER_POT)
    force = pixelNutSupport.clipValue(pc_trigger.newValue, 0, MAX_FORCE_VALUE);
    #else
    uint16_t msecs = bc_trigger.msecsPressed;
    // 1 second press == MAX_FORCE_VALUE
    if (msecs > MAX_FORCE_VALUE)
         force = MAX_FORCE_VALUE;
    else force = msecs;
    #endif

    DBGOUT((F("Trigger button: force=%d"), force));
    pPixelNutEngine->triggerForce(force);
  }
}

static void SetupTriggerButton(void)
{
  // can adjust button settings here...
}

#endif
//========================================================================================

// initialize controls
void SetupTriggerControls(void)
{
  #if defined(DPIN_TRIGGER_BUTTON)
  SetupTriggerButton();
  #endif
}

// called every control loop
void CheckTriggerControls(void)
{
  #if defined(DPIN_TRIGGER_BUTTON) || defined(APIN_TRIGGER_POT)
  CheckTriggerButton();
  #endif
}

//========================================================================================
