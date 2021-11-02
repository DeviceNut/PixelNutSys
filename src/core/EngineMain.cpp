// PixelNut Engine Class Implementation of Init and Trigger functions
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file (must also set in main.h)

#include "core.h"
#include "PixelNutPlugin.h"
#include "PixelNutEngine.h"

bool PixelNutEngine::init(uint16_t num_pixels, byte num_bytes,
                          byte num_layers, byte num_tracks,
                          uint16_t first_pixel, bool backwards)
{
  pixelBytes = num_pixels * num_bytes;

  // allocate track and layer stacks; add 1 for use in swapping
  pluginLayers  = (PluginLayer*)malloc((num_layers + 1) * sizeof(PluginLayer));
  pluginTracks  = (PluginTrack*)malloc((num_tracks + 1) * sizeof(PluginTrack));
  if ((pluginLayers == NULL) || (pluginTracks == NULL)) return false;

  // allocate pixel buffer for each track
  for (int i = 0; i < num_tracks; ++i)
  {
    byte *pbuff = (byte *)malloc(pixelBytes);
    if (pbuff == NULL) return false;
    pluginTracks[i].pBuffer = pbuff;
  }

  // allocate single display pixel buffer
  pDisplayPixels = (byte*)malloc(pixelBytes);
  if (pDisplayPixels == NULL) return false;
  memset(pDisplayPixels, 0, pixelBytes);

  // customize engine settings:

  numPixels         = num_pixels;
  numBytesPerPixel  = num_bytes;
  firstPixel        = first_pixel;
  goBackwards       = backwards;

  maxPluginLayers = (short)num_layers; // swap layer at this index
  maxPluginTracks = (short)num_tracks; // swap track at this index

  pDrawPixels = pDisplayPixels;
  return true;
}

// called by all of the following triggering functions
void PixelNutEngine::triggerLayer(byte layer, short force)
{
  PluginLayer *pLayer = (pluginLayers + layer);
  if (pLayer->disable) return;

  PluginTrack *pTrack = pLayer->pTrack;

  DBGOUT((F("Trigger: layer=%d force=%d"), layer, force));

  short pixCount = 0;
  short degreeHue = 0;
  byte pcentWhite = 0;

  // prevent filter effects from overwriting properties if in extern mode
  if (externPropMode)
  {
    pixCount = pTrack->draw.pixCount;
    degreeHue = pTrack->draw.degreeHue;
    pcentWhite = pTrack->draw.pcentWhite;
  }

  byte *dptr = pDrawPixels;
  // prevent drawing if filter effect
  pDrawPixels = (pLayer->redraw ? pTrack->pBuffer : NULL);
  pLayer->pPlugin->trigger(this, &pTrack->draw, force);
  pDrawPixels = dptr; // restore to the previous value

  if (externPropMode) RestorePropVals(pTrack, pixCount, degreeHue, pcentWhite);

  // if this is the drawing effect for the track then redraw immediately
  if (pLayer->redraw) pTrack->msTimeRedraw = pixelNutSupport.getMsecs();

  pLayer->trigActive = true; // layer has been triggered now
}

// internal: check for any automatic triggering
void PixelNutEngine::RepeatTriger(bool rollover)
{
  for (int i = 0; i <= indexLayerStack; ++i) // for each plugin layer
  {
    if (!pluginLayers[i].pTrack->active) break; // not enabled yet

    // just always reset trigger time after rollover event
    if (rollover && (pluginLayers[i].trigType & TrigTypeBit_Repeating))
      pluginLayers[i].trigTimeMsecs = msTimeUpdate;

    // if repeat triggering is set and have count (or infinite) and time has expired
    if ((pluginLayers[i].trigType & TrigTypeBit_Repeating)               &&
        (pluginLayers[i].trigDnCounter || !pluginLayers[i].trigRepCount) &&
        (pluginLayers[i].trigTimeMsecs <= msTimeUpdate))
    {
      DBGOUT((F("RepeatTrigger: offset=%u range=%d counts=%d:%d"),
                pluginLayers[i].trigRepOffset, pluginLayers[i].trigRepRange,
                pluginLayers[i].trigRepCount, pluginLayers[i].trigDnCounter));

      short force = ((pluginLayers[i].trigForce >= 0) ? 
                      pluginLayers[i].trigForce : random(0, MAX_FORCE_VALUE+1));

      triggerLayer(i, force);

      pluginLayers[i].trigTimeMsecs = msTimeUpdate +
          (1000 * random(pluginLayers[i].trigRepOffset,
                        (pluginLayers[i].trigRepOffset +
                        pluginLayers[i].trigRepRange+1)));

      if (pluginLayers[i].trigDnCounter > 0) --pluginLayers[i].trigDnCounter;
    }
  }
}

// external: cause trigger if enabled in layer
void PixelNutEngine::triggerForce(short force)
{
  for (int i = 0; i <= indexLayerStack; ++i)
    if (pluginLayers[i].trigType & TrigTypeBit_External)
      triggerLayer(i, force);
}

// internal: called from effect plugins
void PixelNutEngine::triggerForce(byte layer, short force)
{
  for (int i = 0; i <= indexLayerStack; ++i)
    if ((pluginLayers[i].trigType & TrigTypeBit_Internal) &&
        (pluginLayers[i].trigLayerIndex == layer))
      triggerLayer(i, force);
}
