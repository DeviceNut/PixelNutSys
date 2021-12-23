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

void PixelNutEngine::clearStacks(void)
{
  DBGOUT((F("Clear stacks: tracks=%d layers=%d"), indexTrackStack, indexLayerStack));

  for (int i = 0; i < indexLayerStack; ++i)
    delete pluginLayers[i].pPlugin;

  indexLayerStack = -1;
  indexTrackStack = -1;
  patternEnabled  = false;

  // clear all pixels and force redisplay
  memset(pDisplayPixels, 0, pixelBytes);
  msTimeUpdate = 0;
}

void PixelNutEngine::ShowAllStacks(void)
{
  DBGOUT((F("Tracks:")));
  for (int i = 0; i <= indexTrackStack; ++i)
  {
    PluginTrack *pTrack = TRACK_MAKEPTR(i);
    DBGOUT((F("  %d: layer=%d count=%d"), i, (pTrack->pLayer - pluginLayers), pTrack->lcount));
  }  

  DBGOUT((F("Layers:")));
  PluginLayer *pLayer = pluginLayers;
  for (int i = 0; i <= indexLayerStack; ++i, ++pLayer)
  {
    DBGOUT((F("  %d: track=%d redraw=%d"), i, TRACK_INDEX(pLayer->pTrack), pLayer->redraw));
  }  
}

// call after changing number of layers or ordering of tracks:
// for each redraw layer: update the layer pointer in its track
void PixelNutEngine::UpdateLayerPtrInTracks(void)
{
  PluginLayer *pLayer = pluginLayers;
  for (int i = 0; i <= indexLayerStack; ++i, ++pLayer)
  {
    if (pLayer->redraw)
    {
      DBGOUT((F("Update track=%d layer: %d => %d"),
              TRACK_INDEX(pLayer->pTrack),
              LAYER_INDEX(pLayer->pTrack->pLayer), i));

      pLayer->pTrack->pLayer = pLayer;
    }
  }
}

// call after changing number of tracks or ordering of layers:
// for each track: update track pointer in each layer for that track
void PixelNutEngine::UpdateTrackPtrInLayers(void)
{
  for (int i = 0; i <= indexTrackStack; ++i)
  {
    PluginTrack *pTrack = TRACK_MAKEPTR(i);
    PluginLayer *pLayer = pTrack->pLayer;

    for (int j = 0; j < pTrack->lcount; ++j, ++pLayer)
    {
      DBGOUT((F("Update layer=%d track: %d => %d"),
                LAYER_INDEX(pLayer), TRACK_INDEX(pLayer->pTrack), i));

      pLayer->pTrack = pTrack;
    }
  }
}

// moves stack memory safely
void PixelNutEngine::ShiftStack(bool dolayer, int isrc, int iend, int idst)
{
  byte *base = (dolayer ? (byte*)pluginLayers : (byte*)pluginTracks);
  int size   = (dolayer ? LAYER_BYTES : TRACK_BYTES);

  DBGOUT((F("Shift %s: (%d,%d) => %d"), dolayer ? "layers" : "tracks", isrc, iend, idst));

  int mlen = size * (iend - isrc + 1);
  byte *psrc = base + (size * isrc);
  byte *pdst = base + (size * idst);
  memmove(pdst, psrc, mlen);
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
  // clear track and pixel buffer
  memset(pTrack, 0, TRACK_BYTES);

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

  ShowAllStacks();

  PixelNutPlugin *pPlugin;
  Status rc = MakeNewPlugin(iplugin, &pPlugin);
  if (rc != Status_Success) return rc;

  ++indexLayerStack; // stack another effect layer
  PluginLayer *pLayer = (pluginLayers + indexLayerStack);

  if (redraw) ++indexTrackStack; // create another effect track
  PluginTrack *pTrack = TRACK_MAKEPTR(indexTrackStack);

  DBGOUT((F("Append: stacks=(track=%d layer=%d)"), indexTrackStack, indexLayerStack));

  if (redraw) InitPluginTrack(pTrack, pLayer);
  ++(pTrack->lcount); // one more layer to track

  InitPluginLayer(pLayer, pTrack, pPlugin, iplugin,  redraw);

  ShowAllStacks();

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
  if (pLayer->redraw) layer += (pLayer->pTrack->lcount - 1);

  if (lastlayer >= indexLayerStack)
    return AppendPluginLayer(iplugin);

  bool redraw = pPluginFactory->pluginDraws(iplugin);

  DBGOUT((F("Add plugin: #%d redraw=%d layer=%d"), iplugin, redraw, layer));

  ShowAllStacks();

  PixelNutPlugin *pPlugin;
  Status rc = MakeNewPlugin(iplugin, &pPlugin);
  if (rc != Status_Success) return rc;

  PluginTrack *pTrack = pLayer->pTrack;
  int lcount = 1;

  if (redraw)
  {
    lcount = pTrack->lcount;
    int track = TRACK_INDEX(pTrack);

    // open space in track stack for one new track
    ShiftStack(false, track, indexTrackStack, (track + 1));

    ++indexTrackStack; // more more effect on the stack
    InitPluginTrack(pTrack, pLayer);

    // this iterates tracks, so they must all be initialized
    UpdateTrackPtrInLayers(); // adjust layers for moved tracks
  }

  // open space in layer stack for all layers in the track
  ShiftStack(true, layer, indexLayerStack, (layer + lcount));

  ++indexLayerStack; // now have another effect layer
  InitPluginLayer(pLayer, pTrack, pPlugin, iplugin, redraw);
  ++(pTrack->lcount); // one more layer to track

  // this iterates layers, so they must all be initialized
  UpdateLayerPtrInTracks(); // adjust tracks for moved layers

  ShowAllStacks();

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

  // clear pixel buffer if this is a redraw layer
  if (redraw) memset(TRACK_BUFFER(pLayer->pTrack), 0, pixelBytes);

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
  PluginLayer *pLayer = (pluginLayers + layer);

  DBGOUT((F("Delete plugin: #%d layer=%d"), pLayer->iplugin, layer));

  delete pLayer->pPlugin;

  ShowAllStacks();

  if (pLayer->redraw)
  {
    int lcount = pLayer->pTrack->lcount;
    int track = TRACK_INDEX(pLayer->pTrack);

    DBGOUT((F("Delete: lcount=%d layers end=%d last=%d"), lcount, (layer + lcount - 1), indexLayerStack));

    if ((layer + lcount - 1) < indexLayerStack)
    {
      // close space in layer stack taken by all layers in the track
      ShiftStack(true, (layer + lcount), indexLayerStack, layer);

      indexLayerStack -= lcount;
      UpdateLayerPtrInTracks(); // adjust tracks for moved layers

      // close space in track stack for that one track
      ShiftStack(false, (track + 1), indexTrackStack, track);

      --indexTrackStack;
      UpdateTrackPtrInLayers(); // adjust layers for moved tracks
    }
    else
    {
      indexLayerStack -= lcount;
      --indexTrackStack;
    }
  }
  else if (layer < indexLayerStack)
  {
    --(pLayer->pTrack->lcount); // decrement before lose pointer to the track

    // close space in layer stack for one layer
    ShiftStack(true, (layer + 1), indexLayerStack, layer);

    --indexLayerStack;
    UpdateLayerPtrInTracks(); // adjust tracks for moved layers
  }
  else
  {
    --indexLayerStack;
    --(pLayer->pTrack->lcount);
  }

  ShowAllStacks();
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

  int cnt_rotate = redraw ? pLayer->pTrack->lcount : 1;
  int cnt_tomove = redraw ? (pLayer + cnt_rotate)->pTrack->lcount : 1;
  int endlayer = layer + cnt_rotate + cnt_tomove - 1;

  DBGOUT((F("Swap layers: (%d,%d) <> (%d,%d)"), layer, cnt_rotate,
            (layer + cnt_rotate), cnt_tomove));

  // MUST have layer(s) after this one in order to swap
  if ((layer + cnt_rotate - 1) >= indexLayerStack)
  {
    DBGOUT((F("No layer above %d to swap"), layer));
    return Status_Error_BadVal;
  }

  ShowAllStacks();

  if (redraw) // rotate the track first
  {
    int track = TRACK_INDEX(pLayer->pTrack);

    // move one track to the saved slot above the usable part of the stack
    ShiftStack(false, track, track, maxPluginTracks);

    // shift one track down to replace the one just moved
    ShiftStack(false, (track + 1), (track + 1), track);

    // move saved track to the opened slot above previous shift
    ShiftStack(false, maxPluginTracks, maxPluginTracks, (track + 1));

    UpdateLayerPtrInTracks(); // adjust layer pointers in moved tracks
  }

  for (int i = 0; i < cnt_rotate; ++i)
  {
    // move one layer to the saved slot above the usable part of the stack
    ShiftStack(true, layer, layer, maxPluginLayers);

    // shift one or more layers down to replace the layer just moved
    ShiftStack(true, (layer + 1), endlayer, layer);

    // move saved layer to the opened slot above previous shift
    ShiftStack(true, maxPluginLayers, maxPluginLayers, endlayer);
  }

  UpdateTrackPtrInLayers(); // adjust track pointers in moved layers

  ShowAllStacks();

  return Status_Success;
}
