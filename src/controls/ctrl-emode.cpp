// Hardware Extern Mode Controls
// Selects the external mode state in the pixelnut engine from physical controls.
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"
#include "UIDeviceButton.h"

#if defined(DPIN_EMODE_BUTTON)

static void SetExternMode(bool enable)
{
  DBGOUT((F("Extern property mode = %d"), enable));
  pPixelNutEngine->setPropertyMode(enable);
}

// don't allow double-click, repeating, or long press
UIDeviceButton bc_emode(DPIN_EMODE_BUTTON);
static bool emode = false; // start with extern mode off

static void CheckEModeButton(void)
{
  if (bc_emode.CheckForChange() != UIDeviceButton::Retcode_NoChange)
    SetExternMode( emode = !emode );
}

static void SetupEModeButton(void)
{
  // can adjust button settings here...
}

#endif
//========================================================================================

// initialize controls
void SetupEModeControls(void)
{
  #if defined(DPIN_EMODE_BUTTON)
  SetupEModeButton();
  #endif
}

// called every control loop
void CheckEModeControls(void)
{
  #if defined(DPIN_EMODE_BUTTON)
  CheckEModeButton();
  #endif
}

//========================================================================================
