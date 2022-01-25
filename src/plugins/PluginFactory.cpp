// PixelNutApp Plugin Factory
// Class to create instances of plugins.
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#include <Arduino.h>

#include "config.h" // app configuration

#include "PixelNutSupport.h"
#include "PixelNutPlugin.h"
#include "PixelNutEngine.h"

// Built-in PixelNut-Plugins (PNP):

#include "PNP_DrawAll.h"
#include "PNP_DrawPush.h"
#include "PNP_DrawStep.h"
#include "PNP_LightWave.h"
#include "PNP_CometHeads.h"
#include "PNP_FerrisWheel.h"
#include "PNP_BlockScanner.h"
#include "PNP_Twinkle.h"
#include "PNP_Blinky.h"
#include "PNP_Noise.h"
#include "PNP_HueSet.h"
#include "PNP_HueRotate.h"
#include "PNP_ColorMeld.h"
#include "PNP_ColorModify.h"
#include "PNP_ColorRandom.h"
#include "PNP_CountSet.h"
#include "PNP_CountWave.h"
#include "PNP_CountSurge.h"
#include "PNP_DelaySet.h"
#include "PNP_DelaySurge.h"
#include "PNP_DelayWave.h"
#include "PNP_BrightSurge.h"
#include "PNP_BrightWave.h"
#include "PNP_WinExpander.h"
#include "PNP_FlipDirection.h"

// returns true if plugin redraws, else filter
bool PluginFactory::pluginDraws(uint16_t plugin)
{
  return (plugin < 100);
}

PixelNutPlugin *PluginFactory::pluginCreate(uint16_t plugin)
{
  switch (plugin)
  {
    // drawing effects:

    case 0:   return new PNP_DrawAll;                     // draws current color to all pixels on each step
    case 1:   return new PNP_DrawPush;                    // draws current color to one pixel on each step, inserting at the end, then clearing
    case 2:   return new PNP_DrawStep;                    // draws current color to one pixel on each step, appending at the start

    case 10:  return new PNP_LightWave;                   // light waves (brighness changes) that move; count property sets wave frequency
    case 20:  return new PNP_CometHeads;                  // creates "comets": moving head with tail that fades, trigger creates new head
    case 30:  return new PNP_FerrisWheel;                 // rotates "ferris wheel spokes" around; count property sets spaces between spokes
    case 40:  return new PNP_BlockScanner;                // moves color block back and forth; count property sets the block length 

                                                          // these use the current color, and count property sets the value of 'N':
    case 50:  return new PNP_Twinkle;                     // scales the brightness levels for 'N' pixels up and down individually
    case 51:  return new PNP_Blinky;                      // blinks 'N' random pixels on/off using current color and brightness
    case 52:  return new PNP_Noise;                       // sets 'N' random pixels using current color and random brightness 

    // predraw effects:

    case 100: return new PNP_HueSet;                      // force directly sets the color hue property value once when triggered
    case 101: return new PNP_HueRotate;                   // rotates color hue on each step; amount of change set from trigger force

    case 110: return new PNP_ColorMeld;                   // smoothly melds between colors when they change
    case 111: return new PNP_ColorModify;                 // force modifies both the color hue/white properties once when triggered
    case 112: return new PNP_ColorRandom;                 // sets color hue/white to random values on each step (doesn't use force)
    
    case 120: return new PNP_CountSet;                    // force directly sets the count property value once when triggered
    case 121: return new PNP_CountSurge;                  // force increases count then evenly reverts to original value
    case 122: return new PNP_CountWave;                   // force determines the number of steps that modulates pixel count

    case 130: return new PNP_DelaySet;                    // force directly sets the delay property value once when triggered
    case 131: return new PNP_DelaySurge;                  // force decreases delay then reverts to original value, must be triggered
    case 132: return new PNP_DelayWave;                   // force determines the number of steps that modulates delay time

    case 141: return new PNP_BrightSurge;                 // force increases brightness, then reverts to original value, must be triggered
    case 142: return new PNP_BrightWave;                  // force determines the number of steps that modulates brightness

    case 150: return new PNP_WinExpander;                 // expands/contracts drawing window that stays centered on strip

    case 160: return new PNP_FlipDirection;               // toggles the drawing direction on each trigger

    default:  return NULL;
  }
}

// must provide destructor for plugin abstract (interface) base class
PixelNutPlugin::~PixelNutPlugin() {}
