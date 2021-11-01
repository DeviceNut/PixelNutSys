// PixelNut Engine Class Implementation of Stack Management
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 1 // 1 enables debugging this file (must also set in main.h)

#include "core.h"
#include "PixelNutPlugin.h"
#include "PixelNutEngine.h"

extern PluginFactory *pPluginFactory; // used to create effect plugins

// update the track pointers in all layers
void PixelNutEngine::updateTrackPtrs(void)
{
  PluginLayer *pLayer = pluginLayers;
  for (int i = 0; i <= indexLayerStack; ++i, ++pLayer)
    pLayer->pTrack->pLayer = pLayer;
}

// update the layer pointers in all track;
void PixelNutEngine::updateLayerPtrs(void)
{
  PluginTrack *pTrack = pluginTracks;
  for (int i = 0; i <= indexTrackStack; ++i, ++pTrack)
    pTrack->pLayer->pTrack = pTrack;
}

// moves stack memory safely
static void shiftStack(byte *base, int isrc, int idst, int iend, int size)
{
  int mlen = size * (iend - isrc + 1);
  byte *psrc = base + (size * isrc);
  byte *pdst = base + (size * idst);
  memmove(pdst, psrc, mlen);
}

void PixelNutEngine::clearStacks(void)
{
  DBGOUT((F("Clear stack: tracks=%d layers=%d"), indexTrackStack, indexLayerStack));

  for (int i = 0; i < indexLayerStack; ++i)
    delete pluginLayers[i].pPlugin;

  indexTrackEnable = -1;
  indexLayerStack  = -1;
  indexTrackStack  = -1;

  // clear all pixels and force redisplay
  memset(pDisplayPixels, 0, pixelBytes);
  msTimeUpdate = 0;
}

// return false if cannot create another plugin, either because
// not enought layers left, or because plugin index is invalid
PixelNutEngine::Status PixelNutEngine::MakeNewPlugin(int iplugin, PixelNutPlugin **ppPlugin, int *ptype)
{
  // check if can add another layer to the stack
  if ((indexLayerStack+1) >= maxPluginLayers)
  {
    DBGOUT((F("Cannot add another layer: max=%d"), (indexLayerStack+1)));
    return Status_Error_Memory;
  }

  *ppPlugin = pPluginFactory->makePlugin(iplugin);
  if (*ppPlugin == NULL) return Status_Error_BadVal;

  *ptype = (*ppPlugin)->gettype();
  bool redraw = (*ptype & PLUGIN_TYPE_REDRAW);

  // check if:
  // a filter plugin and there is at least one redraw plugin, or
  // a redraw plugin and cannot add another track to the stack
  if ((!redraw && (indexTrackStack < 0)) ||
      ( redraw && ((indexTrackStack+1) >= maxPluginTracks)))
  {
    delete *ppPlugin;

    if (redraw)
    {
      DBGOUT((F("Reached max track: max=%d"), (indexTrackStack+1)));
      return Status_Error_Memory;
    }
    else
    {
      DBGOUT((F("Expecting drawing plugin: #%d"), iplugin));
      return Status_Error_BadCmd;
    }
  }

  return Status_Success;
}

void PixelNutEngine::InitPluginLayer(PluginLayer *pLayer, bool redraw)
{
  memset(pLayer, 0, sizeof(PluginLayer));
  pLayer->thisLayerID = uniqueLayerID++;

  PluginTrack *pTrack;

  if (pLayer->redraw)
  {
    ++indexTrackStack; // create another effect track
    pTrack = (pluginTracks + indexTrackStack);

    // clear track but retain pixel buffer
    byte *pbuff = pTrack->pBuffer;
    memset(pTrack, 0, sizeof(PluginTrack));
    pTrack->pBuffer = pbuff;

    pTrack->pLayer = pLayer;  // refer back to this layer

    // initialize track drawing properties to default values
    PixelNutSupport::DrawProps *pProps = &pTrack->draw;
    pProps->pixLen = numPixels; // set initial window
    pProps->pixCount = pixelNutSupport.mapValue(DEF_PCENTCOUNT, 0, MAX_PERCENTAGE, 1, numPixels);

    SETVAL_IF_NONZERO(pProps->pcentBright, DEF_PCENTBRIGHT);
    SETVAL_IF_NONZERO(pProps->pcentDelay,  DEF_PCENTDELAY);
    SETVAL_IF_NONZERO(pProps->degreeHue,   DEF_DEGREESHUE);
    SETVAL_IF_NONZERO(pProps->pcentWhite,  DEF_PCENTWHITE);
    SETVAL_IF_NONZERO(pProps->goBackwards, DEF_BACKWARDS);
    SETVAL_IF_NONZERO(pProps->pixOrValues, DEF_PIXORVALS);

    pixelNutSupport.makeColorVals(pProps); // create RGB values
  }
  else pTrack = (pluginTracks + indexTrackStack); // current track

  pTrack->lcount++; // first/another layer for this track

  pLayer->pTrack  = pTrack;
  pLayer->pPlugin = pLayer->pPlugin;
  pLayer->iplugin = pLayer->iplugin;
  pLayer->redraw  = redraw;

  SETVAL_IF_NONZERO(pLayer->trigForce,     DEF_FORCEVAL);
  SETVAL_IF_NONZERO(pLayer->trigRepCount,  DEF_TRIG_FOREVER);
  SETVAL_IF_NONZERO(pLayer->trigRepOffset, DEF_TRIG_OFFSET);
  SETVAL_IF_NONZERO(pLayer->trigRepRange,  DEF_TRIG_RANGE);
  // all other parameters have been initialized to 0

  // begin new plugin, but will not be drawn until triggered
  pLayer->pPlugin->begin(pLayer->thisLayerID, numPixels);

  // must clear buffer to remove currently drawn pixels
  if (redraw) memset(pTrack->pBuffer, 0, pixelBytes);
}

// Return false if unsuccessful for any reason.
PixelNutEngine::Status PixelNutEngine::AppendPluginLayer(int iplugin)
{
  int ptype;
  PixelNutPlugin *pPlugin;
  Status rc = MakeNewPlugin(iplugin, &pPlugin, &ptype);
  if (rc != Status_Success) return rc;

  bool redraw = !!(ptype & PLUGIN_TYPE_REDRAW);

  ++indexLayerStack; // stack another effect layer
  PluginLayer *pLayer = (pluginLayers + indexLayerStack);

  DBGOUT((F("Append plugin: #%d type=0x%02X track=%d layer=%d"),
          iplugin, ptype, indexTrackStack, indexLayerStack));

  InitPluginLayer(pLayer, redraw);
  return Status_Success;
}

// Move stack layer memory up one layer starting at the specified layer,
// or all the layers that make up a track if this is a track plugin.
//
// Initialize this new layer with the requested plugin, then update the
// layer pointers for each track.
//
// If this is a track plugin, then move the track memory up one track after
// the current track, then update the track pointers for each layer.
//
// Return false if unsuccessful for any reason.
PixelNutEngine::Status PixelNutEngine::InsertPluginLayer(short layer, int iplugin)
{
  int ptype;
  PixelNutPlugin *pPlugin;
  Status rc = MakeNewPlugin(iplugin, &pPlugin, &ptype);
  if (rc != Status_Success) return rc;

  PluginLayer *pLayer = (pluginLayers + layer);

  bool redraw = !!(ptype & PLUGIN_TYPE_REDRAW);
  if (redraw != pLayer->redraw)
  {
    DBGOUT((F("Unexpected plugin(%d) type=%02X for layer=%d"), iplugin, ptype, layer));
    delete pPlugin;
    return Status_Error_BadVal;
  }

  DBGOUT((F("Insert plugin: #%d type=0x%02X layer=%d"), iplugin, ptype, layer));

  if (redraw)
  {
    int lcount = pLayer->pTrack->lcount;
    int track = (pLayer->pTrack - pluginTracks);

    // open space in layer stack for all layers in the track
    shiftStack((byte*)pluginLayers, layer, (layer + lcount), indexLayerStack, sizeof(PluginLayer));
    updateTrackPtrs(); // adjust tracks for moved layers

    // open space in track stack for one new track
    shiftStack((byte*)pluginTracks, track, track+1, indexTrackStack, sizeof(PluginTrack));
    updateLayerPtrs(); // adjust layers for moved tracks
  }
  else
  {
    // open space in layer stack for one new layer
    shiftStack((byte*)pluginLayers, layer, layer+1, indexLayerStack, sizeof(PluginLayer));
    updateTrackPtrs(); // adjust tracks for moved layers
  }

  InitPluginLayer(pLayer, redraw);
  return Status_Success;
}

// Switch plugin effects for the requested layer.
// Return false if unsuccessful for any reason.
PixelNutEngine::Status PixelNutEngine::SwitchPluginLayer(short layer, int iplugin)
{
  int ptype;
  PixelNutPlugin *pPlugin;
  Status rc = MakeNewPlugin(iplugin, &pPlugin, &ptype);
  if (rc != Status_Success) return rc;

  PluginLayer *pLayer = (pluginLayers + layer);
  bool redraw = (ptype & PLUGIN_TYPE_REDRAW);

  if (redraw != pLayer->redraw)
  {
    DBGOUT((F("Unexpected plugin type for layer=%d"), layer));
    return Status_Error_BadVal;
  }

  delete pLayer->pPlugin;
  pLayer->pPlugin = pPlugin;
  pLayer->iplugin = iplugin;

  DBGOUT((F("Switch plugin: #%d type=0x%02X layer=%d"), iplugin, ptype, layer));

  // begin new plugin, but will not be drawn until triggered
  pPlugin->begin(pLayer->thisLayerID, numPixels);

  // must clear buffer to remove current drawn pixels
  memset(pLayer->pTrack->pBuffer, 0, pixelBytes);

  // reset trigger state, then check if should trigger now
  pLayer->trigActive = false;
  if (pLayer->trigType & TrigTypeBit_AtStart)
  {
    short force = pLayer->trigForce;
    if (force < 0) force = random(0, MAX_FORCE_VALUE+1);
    triggerLayer(layer, force);
  }

  return Status_Success;
}

// Copy the memory for the layer(s) to swap (if a track layer
// then all the layers for that track), move the memory down,
// then copy the layer memory to its new location. Then update
// the layer pointers for each track.
//
// If this is a track layer, copy/move/copy the track memory,
// then update all the track pointers for each layer.
void PixelNutEngine::SwapPluginLayers(short layer)
{
  //PluginLayer *pLayer = (pluginLayers + layer);

  DBGOUT((F("Swap layers: %d <=> %d"), layer, layer+1));

}

// If layer/track is the last one, then just update the current
// end-of-stack index (track/layer count), otherwise:
//
// Move stack layer memory down one layer, overwriting specified layer.
// If this is the track layer, move all the layers for this track and
// update the layer pointers for each track. 
//
// If this is the track layer, then move the track memory down one track
// as well, then decrement the track count. Finally, update all the track
// pointers for each layer.
//
// Finally, decrement the layer count by the number of layers deleted;

void PixelNutEngine::DeletePluginLayer(short layer)
{
  DBGOUT((F("Delete plugin: layer=%d"), layer));

  PluginLayer *pLayer = (pluginLayers + layer);
  delete pLayer->pPlugin;

  int lcount = 1;

  if (pLayer->redraw)
  {
    lcount = pLayer->pTrack->lcount;
    int track = (pLayer->pTrack - pluginTracks);

    if ((layer + lcount) < indexLayerStack)
    {
      // close space in layer stack taken by all layers in the track
      shiftStack((byte*)pluginLayers, (layer + lcount), layer, indexLayerStack, sizeof(PluginLayer));
      updateTrackPtrs(); // adjust tracks for moved layers

      // close space in track stack for that one track
      shiftStack((byte*)pluginTracks, track+1, track, indexTrackStack, sizeof(PluginTrack));
      updateLayerPtrs(); // adjust layers for moved tracks
    }

    --indexTrackStack;
  }
  else if (layer+1 < indexLayerStack)
  {
    // close space in layer stack for one layer
    shiftStack((byte*)pluginLayers, layer+1, layer, indexLayerStack, sizeof(PluginLayer));
  }

  indexLayerStack -= lcount;
}
