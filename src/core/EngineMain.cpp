// PixelNut Engine Class Implementation of Init and Trigger functions
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file

#include "core.h"

bool PixelNutEngine::init(uint16_t num_pixels, byte num_bytes,
                          byte num_layers, byte num_tracks,
                          uint16_t first_pixel, bool backwards)
{
  pixelBytes = num_pixels * num_bytes;

  // allocate track and layer stacks; add 1 for use in swapping
  // track includes pixel buffer after end of structure
  pluginLayers  = (PluginLayer*)malloc((num_layers + 1) * LAYER_BYTES);
  pluginTracks  = (PluginTrack*)malloc((num_tracks + 1) * TRACK_BYTES);
  if ((pluginLayers == NULL) || (pluginTracks == NULL)) return false;

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
void PixelNutEngine::TriggerLayer(PluginLayer *pLayer, byte force)
{
  PluginTrack *pTrack = pLayer->pTrack;

  DBGOUT((F("Trigger: track=%d layer=%d force=%d"), TRACK_INDEX(pTrack), LAYER_INDEX(pLayer), force));

  short pixCount = 0;
  byte dvalueHue = 0;
  byte pcentWhite = 0;

  // prevent filter effects from overwriting properties if in extern mode
  if (externPropMode)
  {
    pixCount = pTrack->draw.pixCount;
    dvalueHue = pTrack->draw.dvalueHue;
    pcentWhite = pTrack->draw.pcentWhite;
  }

  byte *dptr = pDrawPixels;
  // prevent drawing if filter effect
  pDrawPixels = (pLayer->redraw ? TRACK_BUFFER(pTrack) : NULL);
  pLayer->pPlugin->trigger(this, &pTrack->draw, force);
  pDrawPixels = dptr; // restore to the previous value

  if (externPropMode) RestorePropVals(pTrack, pixCount, dvalueHue, pcentWhite);

  // if this is the drawing effect for the track then redraw immediately
  if (pLayer->redraw) pTrack->msTimeRedraw = pixelNutSupport.getMsecs();

  pLayer->trigActive = true; // layer has been triggered at least once now
}

// internal: check for any automatic triggering
void PixelNutEngine::RepeatTriger(bool rollover)
{
  for (int i = 0; i <= indexLayerStack; ++i) // for each plugin layer
  {
    // just always reset trigger time after rollover event
    if (rollover && (pluginLayers[i].trigType & TrigTypeBit_Repeating))
      pluginLayers[i].trigTimeMsecs = msTimeUpdate;

    // if repeat triggering is set and have count (or infinite) and time has expired
    if (!pluginLayers[i].disable &&
        (pluginLayers[i].trigType & TrigTypeBit_Repeating) &&
        (pluginLayers[i].trigDnCounter || !pluginLayers[i].trigRepCount) &&
        (pluginLayers[i].trigTimeMsecs <= msTimeUpdate))
    {
      DBGOUT((F("RepeatTrigger: offset=%u range=%d counts=%d:%d"),
                pluginLayers[i].trigRepOffset, pluginLayers[i].trigRepRange,
                pluginLayers[i].trigRepCount, pluginLayers[i].trigDnCounter));

      byte force = (pluginLayers[i].randForce) ?
                      random(0, MAX_FORCE_VALUE+1) :
                      pluginLayers[i].trigForce;

      TriggerLayer((pluginLayers + i), force);

      pluginLayers[i].trigTimeMsecs = msTimeUpdate +
          (1000 * random(pluginLayers[i].trigRepOffset,
                        (pluginLayers[i].trigRepOffset +
                        pluginLayers[i].trigRepRange+1)));

      if (pluginLayers[i].trigDnCounter > 0) --pluginLayers[i].trigDnCounter;
    }
  }
}

// external: called from client command
void PixelNutEngine::triggerForce(byte force)
{
  for (int i = 0; i <= indexLayerStack; ++i)
    if (!pluginLayers[i].disable &&
        (pluginLayers[i].trigType & TrigTypeBit_External))
      TriggerLayer((pluginLayers + i), force);
}

// internal: called from effect plugins
void PixelNutEngine::triggerForce(uint16_t id, byte force)
{
  for (int i = 0; i <= indexLayerStack; ++i)
    if (!pluginLayers[i].disable &&
        (pluginLayers[i].trigType & TrigTypeBit_Internal) &&
        (pluginLayers[i].trigLayerID == id))
      TriggerLayer((pluginLayers + i), force);
}
