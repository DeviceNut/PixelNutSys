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
// then update all source trigger layers
void PixelNutEngine::updateTrackPtrs(void)
{
  PluginLayer *pLayer = pluginLayers;
  for (int i = 0; i <= indexLayerStack; ++i, ++pLayer)
  {
    pLayer->pTrack->pLayer = pLayer;
    if (pLayer->trigType & TrigTypeBit_Internal)
    {
      // TODO: update source layer

    }
  }
}

// update the layer pointers in all tracks
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
PixelNutEngine::Status PixelNutEngine::MakeNewPlugin(uint16_t iplugin, PixelNutPlugin **ppPlugin)
{
  // check if can add another layer to the stack
  if ((indexLayerStack+1) >= maxPluginLayers)
  {
    DBGOUT((F("Cannot add another layer: max=%d"), (indexLayerStack+1)));
    return Status_Error_Memory;
  }

  bool redraw = pPluginFactory->pluginDraws(iplugin);

  // check if:
  // a redraw plugin and cannot add another track to the stack,
  // or a filter plugin and there is at least one redraw plugin
  if (redraw && ((indexTrackStack+1) >= maxPluginTracks))
  {
    DBGOUT((F("Cannot add another track: max=%d"), (indexTrackStack+1)));
    return Status_Error_Memory;
  }
  else if (!redraw && (indexTrackStack < 0))
  {
    DBGOUT((F("Expecting drawing plugin: #%d"), iplugin));
    return Status_Error_BadCmd;
  }

  *ppPlugin = pPluginFactory->pluginCreate(iplugin);
  if (*ppPlugin == NULL) return Status_Error_BadVal;

  return Status_Success;
}

void PixelNutEngine::InitPluginTrack(PluginTrack *pTrack, PluginLayer *pLayer)
{
  // clear track but retain pixel buffer and clear it
  byte *pbuff = pTrack->pBuffer;
  memset(pTrack, 0, sizeof(PluginTrack));
  memset(pbuff, 0, pixelBytes);
  pTrack->pBuffer = pbuff;

  pTrack->pLayer = pLayer; // this layer not yet initialized

  // initialize track drawing properties to default values
  PixelNutSupport::DrawProps *pProps = &pTrack->draw;
  pProps->pixLen = numPixels; // set initial window
  pProps->pixCount = pixelNutSupport.mapValue(DEF_PCENTCOUNT,
                          0,MAX_PERCENTAGE, 1,numPixels);

  SETVAL_IF_NONZERO(pProps->pcentBright, DEF_PCENTBRIGHT);
  SETVAL_IF_NONZERO(pProps->pcentDelay,  DEF_PCENTDELAY);
  SETVAL_IF_NONZERO(pProps->degreeHue,   DEF_DEGREESHUE);
  SETVAL_IF_NONZERO(pProps->pcentWhite,  DEF_PCENTWHITE);
  SETVAL_IF_NONZERO(pProps->goBackwards, DEF_BACKWARDS);
  SETVAL_IF_NONZERO(pProps->pixOrValues, DEF_PIXORVALS);

  pixelNutSupport.makeColorVals(pProps); // create RGB values
}

void PixelNutEngine::InitPluginLayer(PluginLayer *pLayer, PluginTrack *pTrack,
                        PixelNutPlugin *pPlugin, uint16_t iplugin, bool redraw)
{
  memset(pLayer, 0, sizeof(PluginLayer));
  pLayer->thisLayerID = uniqueLayerID++;

  pLayer->pPlugin = pPlugin;
  pLayer->iplugin = iplugin;
  pLayer->redraw  = redraw;
  pLayer->pTrack  = pTrack;

  SETVAL_IF_NONZERO(pLayer->trigForce,     DEF_FORCEVAL);
  SETVAL_IF_NONZERO(pLayer->trigRepCount,  DEF_TRIG_FOREVER);
  SETVAL_IF_NONZERO(pLayer->trigRepOffset, DEF_TRIG_OFFSET);
  SETVAL_IF_NONZERO(pLayer->trigRepRange,  DEF_TRIG_RANGE);
  // all other parameters have been initialized to 0
}

// begin new plugin and trigger if necessary
void PixelNutEngine::BeginPluginLayer(PluginLayer *pLayer)
{
  pLayer->pPlugin->begin(pLayer->thisLayerID, numPixels);

  if (pLayer->trigType & TrigTypeBit_AtStart)
  {
    short force = pLayer->trigForce;
    if (force < 0) force = random(0, MAX_FORCE_VALUE+1);
    TriggerLayer(pLayer, force);
  }
}

// Return false if unsuccessful for any reason.
PixelNutEngine::Status PixelNutEngine::AppendPluginLayer(uint16_t iplugin)
{
  bool redraw = pPluginFactory->pluginDraws(iplugin);

  DBGOUT((F("Append plugin: #%d redraw=%d"), iplugin, redraw));

  PixelNutPlugin *pPlugin;
  Status rc = MakeNewPlugin(iplugin, &pPlugin);
  if (rc != Status_Success) return rc;

  ++indexLayerStack; // stack another effect layer
  PluginLayer *pLayer = (pluginLayers + indexLayerStack);

  if (redraw) ++indexTrackStack; // create another effect track
  PluginTrack *pTrack = (pluginTracks + indexTrackStack);

  DBGOUT((F("Append: max stacks=(track=%d layer=%d) active=%d"),
          indexTrackStack, indexLayerStack, indexTrackEnable));

  if (redraw) InitPluginTrack(pTrack, pLayer);
  ++pTrack->lcount; // one more layer to track

  InitPluginLayer(pLayer, pTrack, pPlugin, iplugin,  redraw);

  BeginPluginLayer(pLayer);
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
PixelNutEngine::Status PixelNutEngine::AddPluginLayer(short layer, uint16_t iplugin)
{
  PluginLayer *pLayer = (pluginLayers + layer);
  int lastlayer = layer;
  if (pLayer->redraw) layer += pLayer->pTrack->lcount-1;

  if (lastlayer >= indexLayerStack)
    return AppendPluginLayer(iplugin);

  bool redraw = pPluginFactory->pluginDraws(iplugin);

  DBGOUT((F("Add plugin: #%d redraw=%d layer=%d"), iplugin, redraw, layer));

  if (redraw != pLayer->redraw) // must be same as current layer
  {
    DBGOUT((F("Unexpected plugin(%d) for layer=%d"), iplugin, layer));
    return Status_Error_BadVal;
  }

  PixelNutPlugin *pPlugin;
  Status rc = MakeNewPlugin(iplugin, &pPlugin);
  if (rc != Status_Success) return rc;

  PluginTrack *pTrack = pLayer->pTrack;
  int lcount = 1;

  if (redraw)
  {
    lcount = pTrack->lcount;
    int track = (pTrack - pluginTracks);

    // open space in track stack for one new track
    shiftStack((byte*)pluginTracks, track, (track + 1), indexTrackStack, sizeof(PluginTrack));

    ++indexTrackStack; // more more effect on the stack
    InitPluginTrack(pTrack, pLayer);

    // this iterates tracks, so they must all be initialized
    updateLayerPtrs(); // adjust layers for moved tracks
  }

  // open space in layer stack for all layers in the track
  shiftStack((byte*)pluginLayers, layer, (layer + lcount), indexLayerStack, sizeof(PluginLayer));

  ++indexLayerStack; // now have another effect layer
  InitPluginLayer(pLayer, pTrack, pPlugin, iplugin, redraw);

  // this iterates layers, so they must all be initialized
  updateTrackPtrs(); // adjust tracks for moved layers

  DBGOUT((F("Add: max stacks=(track=%d layer=%d) active=%d"),
          indexTrackStack, indexLayerStack, indexTrackEnable));

  BeginPluginLayer(pLayer);
  return Status_Success;
}

// Switch plugin effects for the requested layer.
// Return false if unsuccessful for any reason.
PixelNutEngine::Status PixelNutEngine::SwitchPluginLayer(short layer, uint16_t iplugin)
{
  PluginLayer *pLayer = (pluginLayers + layer);
  bool redraw = pPluginFactory->pluginDraws(iplugin);

  DBGOUT((F("Switch plugin: #%d redraw=%d layer=%d"), iplugin, redraw, layer));

  if (redraw != pLayer->redraw)
  {
    DBGOUT((F("Unexpected plugin(%d) for layer=%d"), iplugin, layer));
    return Status_Error_BadVal;
  }

  PixelNutPlugin *pPlugin;
  Status rc = MakeNewPlugin(iplugin, &pPlugin);
  if (rc != Status_Success) return rc;

  delete pLayer->pPlugin;
  pLayer->pPlugin = pPlugin;
  pLayer->iplugin = iplugin;
  pLayer->trigActive = false;

  // must clear buffer to remove current drawn pixels
  memset(pLayer->pTrack->pBuffer, 0, pixelBytes);

  BeginPluginLayer(pLayer);
  return Status_Success;
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

    if ((layer + lcount - 1) < indexLayerStack)
    {
      DBGOUT((F("Shifting layers for delete")));
      // close space in layer stack taken by all layers in the track
      shiftStack((byte*)pluginLayers, (layer + lcount), layer, indexLayerStack, sizeof(PluginLayer));
      updateTrackPtrs(); // adjust tracks for moved layers

      DBGOUT((F("Shifting tracks for delete")));
      // close space in track stack for that one track
      shiftStack((byte*)pluginTracks, (track + 1), track, indexTrackStack, sizeof(PluginTrack));
      updateLayerPtrs(); // adjust layers for moved tracks
    }

    --indexTrackStack;
    if (indexTrackEnable < indexTrackStack)
        indexTrackEnable = indexTrackStack;
  }
  else if (layer < indexLayerStack)
  {
    DBGOUT((F("Shifting layers for delete")));
    // close space in layer stack for one layer
    shiftStack((byte*)pluginLayers, (layer + 1), layer, indexLayerStack, sizeof(PluginLayer));
    updateTrackPtrs(); // adjust tracks for moved layers
  }
  else pLayer->pTrack->lcount -= 1;

  indexLayerStack -= lcount;

  DBGOUT((F("Delete: max stacks=(track=%d layer=%d) active=%d"),
          indexTrackStack, indexLayerStack, indexTrackEnable));
}

// Copy the memory for the layer(s) to swap (if a track layer
// then all the layers for that track), move the memory down,
// then copy the layer memory to its new location. Then update
// the layer pointers for each track.
//
// If this is a track layer, copy/move/copy the track memory,
// then update all the track pointers for each layer.
PixelNutEngine::Status PixelNutEngine::SwapPluginLayers(short layer)
{
  PluginLayer *pLayer = (pluginLayers + layer);
  bool redraw = pLayer->redraw;
  int track = (pLayer->pTrack - pluginTracks);

  int rotate_count = 1;
  int endlayer = layer;

  if (redraw)
    rotate_count = pLayer->pTrack->lcount;

  // MUST have layer(s) after this one in order to swap
  if ((layer + rotate_count) >= indexLayerStack)
  {
    DBGOUT((F("Cannot swap layers: %d<>%d max=%d"),
            layer, (layer + rotate_count), indexLayerStack));
    return Status_Error_BadVal;
  }

  if (redraw)
    endlayer = layer + (pLayer + rotate_count)->pTrack->lcount - 1;

  DBGOUT((F("Swap layers: %d<>%d-%d"), layer, (layer + rotate_count), endlayer));

  for (int i = 0; i < rotate_count; ++i)
  {
    // move one layer to the saved slot above the usable part of the stack
    shiftStack((byte*)pluginLayers, layer, maxPluginLayers, (layer + 1), sizeof(PluginLayer));

    // shift one or more layers down to replace the layer just moved
    shiftStack((byte*)pluginLayers, (layer + 1), layer, endlayer, sizeof(PluginLayer));

    // move saved layer to the opened slot above previous shift
    shiftStack((byte*)pluginLayers, maxPluginLayers, (endlayer + 1), maxPluginLayers, sizeof(PluginLayer));
  }
  updateTrackPtrs(); // adjust tracks for moved layers

  if (redraw) // rotate one track
  {
    // move one track to the saved slot above the usable part of the stack
    shiftStack((byte*)pluginTracks, track, maxPluginTracks, (track + 1), sizeof(PluginTrack));

    // shift one track down to replace the one just moved
    shiftStack((byte*)pluginTracks, (track + 1), track, (track + 1), sizeof(PluginTrack));

    // move saved track to the opened slot above previous shift
    shiftStack((byte*)pluginTracks, maxPluginTracks, (track + 1), maxPluginLayers, sizeof(PluginTrack));

    updateLayerPtrs(); // adjust layers for moved tracks
  }

  return Status_Success;
}
