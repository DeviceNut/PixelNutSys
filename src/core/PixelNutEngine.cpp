// PixelNut Engine Class Implementation
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 1 // 1 enables debugging this file (must also set in main.h)

#include "core.h"
#include "PixelNutPlugin.h"
#include "PixelNutEngine.h"

extern PluginFactory *pPluginFactory; // use externally declared pointer to instance

////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor: initialize class variables, allocate memory for layer/track stacks
////////////////////////////////////////////////////////////////////////////////////////////////////

PixelNutEngine::PixelNutEngine(byte *ptr_pixels, uint16_t num_pixels,
                               uint16_t first_pixel, bool goupwards,
                               byte num_layers, byte num_tracks)
{
  // NOTE: cannot call DBGOUT here if statically constructed

  pDisplayPixels  = ptr_pixels;
  numPixels       = num_pixels;
  firstPixel      = first_pixel;
  goUpwards       = goupwards;

  maxPluginLayers = (short)num_layers;
  maxPluginTracks = (short)num_tracks;

  pluginLayers = (PluginLayer*)malloc(num_layers * sizeof(PluginLayer));
  pluginTracks = (PluginTrack*)malloc(num_tracks * sizeof(PluginTrack));

  if ((ptr_pixels == NULL) || (num_pixels == 0) ||
    (pluginLayers == NULL) || (pluginTracks == NULL))
       pDrawPixels = NULL; // caller must test for this
  else pDrawPixels = pDisplayPixels;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal string to numeric value handling routines
////////////////////////////////////////////////////////////////////////////////////////////////////

// returns true if value present and > 0 else 'nullval'
static bool GetBoolValue(char *str, bool nullval)
{
  if (*str == '0') return false;
  if (*str == '1') return true;
  return nullval;
}

// returns -1 if no value, or not in range 0-'maxval'
static int GetNumValue(char *str, int maxval)
{
  if ((str == NULL) || !isdigit(*str)) return -1;
  int newval = atoi(str);
  if (newval > maxval) return -1;
  if (newval < 0) return -1;
  return newval;
}

// clips values to range 0-'maxval'
// returns 'curval' if no value is specified
static int GetNumValue(char *str, int curval, int maxval) // FIXME: use signed values
{
  if ((str == NULL) || !isdigit(*str)) return curval;
  int newval = atoi(str);
  if (newval > maxval) return maxval;
  if (newval < 0) return 0;
  return newval;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal stack handling routines
// Both the plugin layer stack and the track drawing stack start off empty, and can be made to be
// completely empty after layers and tracks have been added by repeated use of the Pop(P) command.
////////////////////////////////////////////////////////////////////////////////////////////////////

void PixelNutEngine::clearStack(void)
{
  DBGOUT((F("Clear stack: track=%d layer=%d"), indexTrackStack, indexLayerStack));

  for (int i = indexTrackStack; i >= 0; --i)
  {
    DBGOUT((F("  Layer %d: track=%d"), i, pluginLayers[i].track));

    // delete in reverse order: first the layer plugins
    int count = 0;
    for (int j = pluginTracks[i].layer; j < indexLayerStack; ++j)
    {
      ++count;
      delete pluginLayers[j].pPlugin;
    }
    indexLayerStack -= count;

    // then the track buffer
    if (pluginTracks[i].pRedrawBuff != NULL)
    {
      DBGOUT((F("Freeing pixel buffer: track=%d"), indexTrackStack));
      free(pluginTracks[i].pRedrawBuff);
    }
  }

  indexTrackEnable = -1;
  indexLayerStack  = -1;
  indexTrackStack  = -1;

  // clear all pixels too
  memset(pDisplayPixels, 0, (numPixels*3));
}

// return false if unsuccessful for any reason
PixelNutEngine::Status PixelNutEngine::AddPluginLayer(int iplugin)
{
  // check if can add another layer to the stack
  if ((indexLayerStack+1) >= maxPluginLayers)
  {
    DBGOUT((F("Cannot add another layer: max=%d"), (indexLayerStack+1)));
    return Status_Error_Memory;
  }

  PixelNutPlugin *pPlugin = pPluginFactory->makePlugin(iplugin);
  if (pPlugin == NULL) return Status_Error_BadVal;

  // determine if must allocate buffer for track, or is a filter plugin
  int ptype = pPlugin->gettype();
  bool dodraw = (ptype & PLUGIN_TYPE_REDRAW);
  DBGOUT((F("plugin=%d pytpe=%d"), iplugin, ptype));

  // check if:
  // a filter plugin and there is at least one redraw plugin, or
  // a redraw plugin and cannot add another track to the stack
  if ((!dodraw && (indexTrackStack < 0)) ||
      ( dodraw && ((indexTrackStack+1) >= maxPluginTracks)))
  {
    delete pPlugin;

    if (dodraw)
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

  ++indexLayerStack; // stack another effect layer
  memset(&pluginLayers[indexLayerStack], 0, sizeof(PluginLayer));

  if (dodraw)
  {
    ++indexTrackStack; // create another effect track
    PluginTrack *pTrack = &pluginTracks[indexTrackStack];
    memset(pTrack, 0, sizeof(PluginTrack));

    pTrack->layer = indexLayerStack;
    // Note: all other trigger parameters are initialized to 0

    // initialize track drawing properties to default values
    pTrack->draw.pixLen        = numPixels;       // set initial window (start was memset)
    pTrack->draw.pcentBright   = DEF_PCENTBRIGHT;
    pTrack->draw.msecsDelay    = DEF_DELAYMSECS;
    pTrack->draw.degreeHue     = DEF_DEGREESHUE;
    pTrack->draw.pcentWhite    = DEF_PCENTWHITE;
    pTrack->draw.pixCount      = pixelNutSupport.mapValue(DEF_PCENTCOUNT, 0, MAX_PERCENTAGE, 1, numPixels);
    pTrack->draw.goUpwards     = DEF_UPWARDS;
    pTrack->draw.pixOverwrite  = DEF_PIXOWRITE;
    pixelNutSupport.makeColorVals(&pTrack->draw); // create RGB values
  }

  PluginLayer *pLayer = &pluginLayers[indexLayerStack];
  pLayer->pPlugin         = pPlugin;
  pLayer->iplugin         = iplugin;
  pLayer->redraw          = dodraw;
  pLayer->track           = indexTrackStack;
  pLayer->trigLayer       = MAX_BYTE_VALUE; // disabled
  pLayer->trigForce       = DEF_FORCEVAL;
  pLayer->trigNumber      = DEF_TRIG_COUNT;
  pLayer->trigDelayMin    = DEF_TRIG_DELAY;
  pLayer->trigDelayRange  = DEF_TRIG_RANGE;
  // Note: all other trigger parameters are initialized to 0

  DBGOUT((F("Append plugin: #%d type=0x%02X redraw=%d track=%d layer=%d"),
          iplugin, ptype, dodraw, indexTrackStack, indexLayerStack));

  // begin new plugin, but will not be drawn until triggered
  pPlugin->begin(indexLayerStack, numPixels);

  if (dodraw) // wait to do this until after any memory allocation in plugin
  {
    int numbytes = numPixels*3;
    byte *p = (byte*)malloc(numbytes);

    if (p == NULL)
    {
      DBGOUT((F("!!! Memory alloc for %d bytes failed !!!"), numbytes));
      DBGOUT((F("Restoring stack and deleting plugin")));

      --indexTrackStack;
      --indexLayerStack;
      delete pPlugin;
      return Status_Error_Memory;
    }
    DBG( else DBGOUT((F("Allocated %d bytes for pixel buffer"), numbytes)); )

    memset(p, 0, numbytes);
    pluginTracks[indexTrackStack].pRedrawBuff = p;
  }

  return Status_Success;
}

// return false if unsuccessful for any reason
PixelNutEngine::Status PixelNutEngine::ModPluginLayer(int iplugin, short layer)
{
  PixelNutPlugin *pPlugin = pPluginFactory->makePlugin(iplugin);
  if (pPlugin == NULL) return Status_Error_BadVal;

  PluginLayer *pLayer = &pluginLayers[layer];
  if (pLayer->pPlugin != NULL) delete pLayer->pPlugin;
  pLayer->pPlugin = pPlugin;
  pLayer->iplugin = iplugin;

  DBGOUT((F("Replace plugin: #%d track=%d layer=%d"), iplugin, pLayer->track, layer));

  // begin new plugin, but will not be drawn until triggered
  pPlugin->begin(layer, numPixels);

  // must clear buffer to remove current drawn pixels
  memset(pluginTracks[pLayer->track].pRedrawBuff, 0, numPixels*3);

  // reset trigger state, then check if should trigger now
  pLayer->trigActive = false;
  if (pluginLayers[layer].trigType & TrigTypeBit_AtStart)
  {
    short force = pluginLayers[layer].trigForce;
    if (force < 0) force = random(0, MAX_FORCE_VALUE+1);
    triggerLayer(layer, force);
  }

  return Status_Success;
}

void PixelNutEngine::DelPluginLayer(short track, short layer)
{
  PluginLayer *pLayer = &pluginLayers[layer];

  if (pLayer->pPlugin != NULL)
  {
    DBGOUT((F("Delete plugin: track=%d layer=%d"), track, layer));
    delete pLayer->pPlugin;
    pLayer->pPlugin = NULL;

    if (pluginTracks[track].pRedrawBuff != NULL)
    {
      // must clear buffer to remove current drawn pixels
      memset(pluginTracks[track].pRedrawBuff, 0, numPixels*3);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Trigger force handling routines
////////////////////////////////////////////////////////////////////////////////////////////////////

void PixelNutEngine::triggerLayer(byte layer, short force)
{
  PluginLayer *pLayer = &pluginLayers[layer];
  int track = pLayer->track;
  PluginTrack *pTrack = &pluginTracks[track];

  DBGOUT((F("Trigger: layer=%d track=%d(L%d) force=%d"), layer, track, pTrack->layer, force));

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
  pDrawPixels = (pLayer->redraw ? pTrack->pRedrawBuff : NULL); // prevent drawing if not filter effect
  pLayer->pPlugin->trigger(this, &pTrack->draw, force);
  pDrawPixels = dptr; // restore to the previous value

  if (externPropMode) RestorePropVals(pTrack, pixCount, degreeHue, pcentWhite);

  // if this is the drawing effect for the track then redraw immediately
  if (pLayer->redraw) pTrack->msTimeRedraw = pixelNutSupport.getMsecs();

  pLayer->trigActive = true; // layer has been triggered now
}

// internal: check for any automatic triggering
void PixelNutEngine::CheckAutoTrigger(bool rollover)
{
  for (int i = 0; i <= indexLayerStack; ++i) // for each plugin layer
  {
    if (pluginLayers[i].track > indexTrackEnable) break; // not enabled yet

    // just always reset trigger time after rollover event
    if (rollover && (pluginLayers[i].trigType & TrigTypeBit_Automatic))
      pluginLayers[i].trigTimeMsecs = timePrevUpdate;

    if ((pluginLayers[i].trigType & TrigTypeBit_Automatic)               && // auto-triggering set
       (pluginLayers[i].trigCounter || (pluginLayers[i].trigNumber < 0)) && // have count (or infinite)
       (pluginLayers[i].trigTimeMsecs <= timePrevUpdate))                   // and time has expired
    {
      DBGOUT((F("AutoTrigger: prevtime=%lu msecs=%lu delay=%u+%u number=%d counter=%d"),
                timePrevUpdate, pluginLayers[i].trigTimeMsecs,
                pluginLayers[i].trigDelayMin, pluginLayers[i].trigDelayRange,
                pluginLayers[i].trigNumber, pluginLayers[i].trigCounter));

      short force = ((pluginLayers[i].trigForce >= 0) ? 
                      pluginLayers[i].trigForce : random(0, MAX_FORCE_VALUE+1));

      triggerLayer(i, force);

      pluginLayers[i].trigTimeMsecs = timePrevUpdate +
          (1000 * random(pluginLayers[i].trigDelayMin,
                        (pluginLayers[i].trigDelayMin +
                        pluginLayers[i].trigDelayRange+1)));

      if (pluginLayers[i].trigCounter > 0) --pluginLayers[i].trigCounter;
    }
  }
}

// external: cause trigger if enabled in track
void PixelNutEngine::triggerForce(short force)
{
  for (int i = 0; i <= indexLayerStack; ++i)
  {
    if (!pluginLayers[i].disable &&
        (pluginLayers[i].trigType & TrigTypeBit_External))
    {
      if (pluginLayers[i].trigNumber > 0)
          pluginLayers[i].trigCounter = pluginLayers[i].trigNumber;

      triggerLayer(i, force);
    }
  }
}

// internal: called from plugins
void PixelNutEngine::triggerForce(byte layer, short force, PixelNutSupport::DrawProps *pdraw)
{
  for (int i = 0; i <= indexLayerStack; ++i)
  {
    if (!pluginLayers[i].disable &&
        (pluginLayers[i].trigType & TrigTypeBit_External) &&
        (pluginLayers[i].trigLayer == layer)) // assume MAX_BYTE_VALUE never valid
    {
      if (pluginLayers[i].trigNumber > 0)
          pluginLayers[i].trigCounter = pluginLayers[i].trigNumber;

      triggerLayer(i, force);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw property related routines
////////////////////////////////////////////////////////////////////////////////////////////////////

void PixelNutEngine::setPropertyMode(bool enable)
{
  DBGOUT((F("Engine property mode: %s"), (enable ? "enabled" : "disabled")));
  externPropMode = enable;
}

void PixelNutEngine::SetPropColor(void)
{
  DBGOUT((F("Engine color properties for tracks:")));

  // adjust all tracks that allow extern control with Q command
  for (int i = 0; i <= indexTrackStack; ++i)
  {
    PluginTrack *pTrack = (pluginTracks + i);
    if (pluginLayers[pTrack->layer].disable)
      continue;

    bool doset = false;

    if (pTrack->ctrlBits & ExtControlBit_DegreeHue)
    {
      DBGOUT((F("  %d) hue: %d => %d"), i, pTrack->draw.degreeHue, externDegreeHue));
      pTrack->draw.degreeHue = externDegreeHue;
      doset = true;
    }

    if (pTrack->ctrlBits & ExtControlBit_PcentWhite)
    {
      DBGOUT((F("  %d) whiteness: %d%% => %d%%"), i, pTrack->draw.pcentWhite, externPcentWhite));
      pTrack->draw.pcentWhite = externPcentWhite;
      doset = true;
    }

    if (doset) pixelNutSupport.makeColorVals(&pTrack->draw);
  }
}

void PixelNutEngine::setColorProperty(short hue_degree, byte white_percent)
{
  externDegreeHue = pixelNutSupport.clipValue(hue_degree, 0, MAX_DEGREES_HUE);
  externPcentWhite = pixelNutSupport.clipValue(white_percent, 0, MAX_PERCENTAGE);
  if (externPropMode) SetPropColor();
}

void PixelNutEngine::SetPropCount(void)
{
  DBGOUT((F("Engine pixel count property for tracks:")));

  // adjust all tracks that allow extern control with Q command
  for (int i = 0; i <= indexTrackStack; ++i)
  {
    PluginTrack *pTrack = (pluginTracks + i);
    if (pluginLayers[pTrack->layer].disable)
      continue;

    if (pTrack->ctrlBits & ExtControlBit_PixCount)
    {
      uint16_t count = pixelNutSupport.mapValue(externPcentCount, 0, MAX_PERCENTAGE, 1, numPixels);
      DBGOUT((F("  %d) %d => %d"), i, pTrack->draw.pixCount, count));
      pTrack->draw.pixCount = count;
    }
  }
}

void PixelNutEngine::setCountProperty(byte pixcount_percent)
{
  // clip and map value into a pixel count, dependent on the actual number of pixels
  externPcentCount = pixelNutSupport.clipValue(pixcount_percent, 0, MAX_PERCENTAGE);
  if (externPropMode) SetPropCount();
}

// internal: restore property values for bits set for track
void PixelNutEngine::RestorePropVals(PluginTrack *pTrack,
                      uint16_t pixCount, uint16_t degreeHue, byte pcentWhite)
{
  if (pluginLayers[pTrack->layer].disable) return;

  if (pTrack->ctrlBits & ExtControlBit_PixCount)
    pTrack->draw.pixCount = pixCount;

  bool doset = false;

  if ((pTrack->ctrlBits & ExtControlBit_DegreeHue) &&
      (pTrack->draw.degreeHue != degreeHue))
  {
    //DBGOUT((F(">>hue: %d->%d"), pTrack->draw.degreeHue, degreeHue));
    pTrack->draw.degreeHue = degreeHue;
    doset = true;
  }

  if ((pTrack->ctrlBits & ExtControlBit_PcentWhite) &&
      (pTrack->draw.pcentWhite != pcentWhite))
  {
    //DBGOUT((F(">>wht: %d->%d"), pTrack->draw.pcentWhite, pcentWhite));
    pTrack->draw.pcentWhite = pcentWhite;
    doset = true;
  }

  if (doset) pixelNutSupport.makeColorVals(&pTrack->draw);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Main command handler and pixel buffer renderer
////////////////////////////////////////////////////////////////////////////////////////////////////

// uses all alpha characters except 'S'
PixelNutEngine::Status PixelNutEngine::execCmdStr(char *cmdstr)
{
  Status status = Status_Success;

  short curtrack = indexTrackStack;
  short curlayer = indexLayerStack;

  for (int i = 0; cmdstr[i]; ++i) // convert to upper case
    cmdstr[i] = toupper(cmdstr[i]);

  char *cmd = strtok(cmdstr, " "); // separate options by spaces

  if (cmd == NULL) return Status_Success; // ignore empty line
  do
  {
    PixelNutSupport::DrawProps *pdraw = NULL;
    if (curtrack >= 0) pdraw = &pluginTracks[curtrack].draw;

    DBGOUT((F(">> Cmd=%s len=%d track=%d layer=%d"), cmd, strlen(cmd), curtrack, curlayer));

    if (cmd[0] == 'P') // Pop all plugins from the stack
    {
      clearStack();
      curlayer = curtrack = -1; // must reset these after clear
      timePrevUpdate = 0; // redisplay pixels after being cleared
    }
    else if (cmd[0] == 'L') // set plugin layer to modify ('L' uses top of stack)
    {
      int layer = GetNumValue(cmd+1, indexLayerStack); // returns -1 if not in range
      DBGOUT((F("LayerCmd: %d max=%d"), layer, indexLayerStack));
      if (layer >= 0)
      {
        curlayer = layer;
        curtrack = pluginLayers[curlayer].track;
      }
      else status = Status_Error_BadVal;
    }
    else if (cmd[0] == 'E') // add a plugin Effect to the stack ("E" is an error)
    {
      int plugin = GetNumValue(cmd+1, MAX_PLUGIN_VALUE); // returns -1 if not in range
      if (plugin >= 0)
      {
        status = AddPluginLayer(plugin);
        if (status == Status_Success)
        {
          curtrack = indexTrackStack;
          curlayer = indexLayerStack;
        }
      }
      else status = Status_Error_BadVal;
    }
    else if (pdraw != NULL)
    {
      switch (cmd[0])
      {
        case 'Z': // sets/clears mute state for track/layer TODO
        {
          break;
        }
        case 'S': // switch/remove effect for existing track ("S" deletes entire layer)
        {
          if (isdigit(*(cmd+1))) // there is a value after "S"
          {
            int plugin = GetNumValue(cmd+1, MAX_PLUGIN_VALUE); // returns -1 if not in range
            if (plugin >= 0) status = ModPluginLayer(plugin, curlayer);
            else status = Status_Error_BadVal;
          }
          else DelPluginLayer(curtrack, curlayer);
          break;
        }
        case 'X': // offset into output display of the track by pixel index
        {
          pdraw->pixStart = GetNumValue(cmd+1, 0, numPixels-1);
          DBGOUT((F(">> Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'Y': // number of pixels in the track by pixel index
        {
          pdraw->pixLen = GetNumValue(cmd+1, 0, numPixels);
          DBGOUT((F(">> Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'J': // offset into output display of the track by percent
        {
          pdraw->pixStart = (GetNumValue(cmd+1, 0, MAX_PERCENTAGE) * (numPixels-1)) / MAX_PERCENTAGE;
          DBGOUT((F(">> Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'K': // number of pixels in the track by percent
        {
          pdraw->pixLen = ((GetNumValue(cmd+1, 0, MAX_PERCENTAGE) * (numPixels-1)) / MAX_PERCENTAGE) + 1;
          DBGOUT((F(">> Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'H': // color Hue degrees property ("H" has no effect)
        {
          pdraw->degreeHue = GetNumValue(cmd+1, pdraw->degreeHue, MAX_DEGREES_HUE);
          pixelNutSupport.makeColorVals(pdraw);
          break;
        }
        case 'W': // percent Whiteness property ("W" has no effect)
        {
          pdraw->pcentWhite = GetNumValue(cmd+1, pdraw->pcentWhite, MAX_PERCENTAGE);
          pixelNutSupport.makeColorVals(pdraw);
          break;
        }
        case 'B': // percent brightness property ("B" has no effect)
        {
          pdraw->pcentBright = GetNumValue(cmd+1, pdraw->pcentBright, MAX_PERCENTAGE);
          pixelNutSupport.makeColorVals(pdraw);
          break;
        }
        case 'C': // pixel count property ("C" has no effect)
        {
          short curvalue = ((pdraw->pixCount * MAX_PERCENTAGE) / numPixels);
          short percent = GetNumValue(cmd+1, curvalue, MAX_PERCENTAGE);
          //DBGOUT((F("CurCount: %d==%d%% numPixels=%d"), pdraw->pixCount, curvalue, numPixels));

          // map value into a pixel count, dependent on the actual number of pixels
          pdraw->pixCount = pixelNutSupport.mapValue(percent, 0, MAX_PERCENTAGE, 1, numPixels);
          DBGOUT((F("PixCount: %d==%d%%"), pdraw->pixCount, percent));
          break;
        }
        case 'D': // drawing delay ("D" has no effect)
        {
          pdraw->msecsDelay = GetNumValue(cmd+1, pdraw->msecsDelay, MAX_DELAY_VALUE);
          break;
        }
        case 'Q': // extern control bits ("Q" has no effect)
        {
          short bits = GetNumValue(cmd+1, ExtControlBit_All); // returns -1 if not within range
          if (bits >= 0)
          {
            pluginTracks[curtrack].ctrlBits = bits;
            if (externPropMode)
            {
              if (bits & ExtControlBit_DegreeHue)
              {
                pdraw->degreeHue = externDegreeHue;
                DBGOUT((F("SetExtern: track=%d hue=%d"), curtrack, externDegreeHue));
              }

              if (bits & ExtControlBit_PcentWhite)
              {
                pdraw->pcentWhite = externPcentWhite;
                DBGOUT((F("SetExtern: track=%d white=%d"), curtrack, externPcentWhite));
              }

              if (bits & ExtControlBit_PixCount)
              {
                pdraw->pixCount = pixelNutSupport.mapValue(externPcentCount, 0, MAX_PERCENTAGE, 1, numPixels);
                DBGOUT((F("SetExtern: track=%d count=%d"), curtrack, pdraw->pixCount));
              }

              pixelNutSupport.makeColorVals(pdraw); // create RGB values
            }
          }
          break;
        }
        case 'U': // drawing direction ("U" to reverse, "U1" for default: go up)
        {
          pdraw->goUpwards = GetBoolValue(cmd+1, false);
          break;
        }
        case 'V': // oVerwrite pixels ("V" to merge, "V1" for default: overwrite)
        {
          pdraw->pixOverwrite = GetBoolValue(cmd+1, false);
          break;
        }
        case 'F': // force value to be used by trigger ("F" means random force)
        {
          if (isdigit(*(cmd+1))) // there is a value after "F" (clip to 0-MAX_FORCE_VALUE)
               pluginLayers[curlayer].trigForce = GetNumValue(cmd+1, 0, MAX_FORCE_VALUE);
          else pluginLayers[curlayer].trigForce = -1; // get random value each time
          break;
        }
        case 'N': // auto trigger counter ("N" or "N0" means forever, same as not specifying at all)
                  // (this count does NOT include the initial trigger from the "T" command)
                  // note that this count gets reset upon each trigger (if not -1)
                  // (clip to 0-MAX_WORD_VALUE)
        {
          pluginLayers[curlayer].trigNumber = GetNumValue(cmd+1, 0, MAX_WORD_VALUE);
          if (!pluginLayers[curlayer].trigNumber) pluginLayers[curlayer].trigNumber = -1;
          else pluginLayers[curlayer].trigCounter = pluginLayers[curlayer].trigNumber;
          break;
        }
        case 'O': // minimum auto-triggering time ("O", "O0", "O1" all get set to default(1sec))
        {
          uint16_t min = GetNumValue(cmd+1, 1, MAX_WORD_VALUE); // clip to 0-MAX_WORD_VALUE, default is 1
          pluginLayers[curlayer].trigDelayMin = min ? min : 1;
          break;
        }
        case 'R': // range of trigger time ("R" means "R0" - default)
        {
          pluginLayers[curlayer].trigDelayRange = GetNumValue(cmd+1, 0, MAX_WORD_VALUE);
          break;
        }
        case 'M': // sets automatic trigger ("M0" to disable)
        {
          if (GetBoolValue(cmd+1, true))
          {
            pluginLayers[curlayer].trigType |= TrigTypeBit_Automatic;
            pluginLayers[curlayer].trigTimeMsecs = pixelNutSupport.getMsecs() +
                (1000 * random(pluginLayers[curlayer].trigDelayMin,
                              (pluginLayers[curlayer].trigDelayMin + pluginLayers[curlayer].trigDelayRange+1)));

            DBGOUT((F("AutoTrigger: layer=%d delay=%u+%u number=%d counter=%d force=%d"), curlayer,
                      pluginLayers[curlayer].trigDelayMin, pluginLayers[curlayer].trigDelayRange,
                      pluginLayers[curlayer].trigNumber, pluginLayers[curlayer].trigCounter,
                      pluginLayers[curlayer].trigForce));
          }
          else pluginLayers[curlayer].trigType &= ~TrigTypeBit_Automatic;
        }
        case 'A': // assign effect layer as trigger source ("A" to disable)
        {
          pluginLayers[curlayer].trigLayer = GetNumValue(cmd+1, MAX_BYTE_VALUE, MAX_BYTE_VALUE);
          DBGOUT((F("Triggering assigned to layer %d"), pluginLayers[curlayer].trigLayer));
          break;
        }
        case 'I': // external triggering enable ('I0' to disable)
        {
          if (GetBoolValue(cmd+1, true))
               pluginLayers[curlayer].trigType |=  TrigTypeBit_External;
          else pluginLayers[curlayer].trigType &= ~TrigTypeBit_External;
          break;
        }
        case 'T': // trigger plugin layer ('T0' to disable)
        {
          if (GetBoolValue(cmd+1, true))
          {
            pluginLayers[curlayer].trigType |= TrigTypeBit_AtStart;
            short force = pluginLayers[curlayer].trigForce;
            if (force < 0) force = random(0, MAX_FORCE_VALUE+1);
            triggerLayer(curlayer, force); // trigger immediately
          }
          else pluginLayers[curlayer].trigType &= ~TrigTypeBit_AtStart;
          break;
        }
        case 'G': // Go: activate newly added effect tracks
        {
          if (indexTrackEnable != indexTrackStack)
          {
            DBGOUT((F("Activate tracks %d to %d"), indexTrackEnable+1, indexTrackStack));
            indexTrackEnable = indexTrackStack;
          }
          break;
        }
        default:
        {
          status = Status_Error_BadCmd;
          break;
        }
      }
    }
    else
    {
      DBGOUT((F("Must add track before setting draw parms")));
      status = Status_Error_BadCmd;
    }

    if (status != Status_Success) break;

    cmd = strtok(NULL, " ");
  }
  while (cmd != NULL);

  DBGOUT((F(">> Exec: status=%d"), status));
  return status;
}

// checks for overflow, returns >0 if successful
static bool addstr(char **pcmdstr, char *addstr, int* pmaxlen)
{
  int slen = strlen(addstr);
  if (slen >= *pmaxlen) return false;
  strcpy(*pcmdstr, addstr);
  *pcmdstr += slen;
  *pmaxlen -= slen;
  return true;
}
// forms command string from current track/layer stacks
bool PixelNutEngine::makeCmdStr(char *cmdstr, int maxlen)
{
  DBG( char *basestr = cmdstr; )
  int addlen = maxlen;
  char str[20];
  *cmdstr = 0;

  PluginLayer *pLayer = pluginLayers;
  for (int i = 0; i <= indexLayerStack; ++i, ++pLayer)
  {
    PluginTrack *pTrack = &pluginTracks[pLayer->track];
    PixelNutSupport::DrawProps *pdraw = &pTrack->draw;

    if (pLayer->disable) continue;

    sprintf(str, "E%d ", pLayer->iplugin);
    if (!addstr(&cmdstr, str, &addlen)) goto error;

    if (i == pTrack->layer) // drawing layer
    {
      if (pdraw->pixStart != 0)
      {
        sprintf(str, "J%d ", pdraw->pixStart);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->pixLen != numPixels)
      {
        sprintf(str, "K%d ", pdraw->pixLen);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->pcentBright != DEF_PCENTBRIGHT)
      {
        sprintf(str, "B%d ", pdraw->pcentBright);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->msecsDelay != DEF_DELAYMSECS)
      {
        sprintf(str, "D%d ", pdraw->msecsDelay);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->degreeHue != DEF_DEGREESHUE)
      {
        sprintf(str, "H%d ", pdraw->degreeHue);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->pcentWhite != DEF_PCENTWHITE)
      {
        sprintf(str, "W%d ", pdraw->pcentWhite);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      int pcent = ((pdraw->pixCount * MAX_PERCENTAGE) / numPixels);
      int count = pixelNutSupport.mapValue(pcent, 0, MAX_PERCENTAGE, 1, numPixels);
      if (pdraw->pixCount != count)
      {
        sprintf(str, "C%d ", pcent);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pTrack->ctrlBits != 0)
      {
        sprintf(str, "Q%d ", pTrack->ctrlBits);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->goUpwards != DEF_UPWARDS)
      {
        sprintf(str, "U ");
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->pixOverwrite != DEF_PIXOWRITE)
      {
        sprintf(str, "V ");
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }
    }

    if (pLayer->trigForce != DEF_FORCEVAL)
    {
      if (pLayer->trigForce < 0) sprintf(str, "F ");
      else sprintf(str, "F%d ", pLayer->trigForce);
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }

    if (pLayer->trigNumber != DEF_TRIG_COUNT)
    {
      if (pLayer->trigNumber < 0) sprintf(str, "N ");
      else sprintf(str, "N%d ", pLayer->trigNumber);
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }

    if (pLayer->trigDelayMin != DEF_TRIG_DELAY)
    {
      sprintf(str, "O%d ", pLayer->trigDelayMin);
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }

    if (pLayer->trigDelayRange != DEF_TRIG_RANGE)
    {
      sprintf(str, "R%d ", pLayer->trigDelayRange);
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }

    if (pLayer->trigType & TrigTypeBit_Automatic)
    {
      sprintf(str, "M ");
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }

    if (pLayer->trigLayer != DEF_TRIG_LAYER)
    {
      sprintf(str, "A%d ", pLayer->trigLayer);
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }

    if (pLayer->trigType & TrigTypeBit_External)
    {
      sprintf(str, "I ");
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }

    if (pLayer->trigType & TrigTypeBit_AtStart)
    {
      sprintf(str, "T ");
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }
    
    DBGOUT((F("Make: track=%d layer=%d plugin=%d str=\"%s\""), pLayer->track, i, pLayer->iplugin, basestr));
  }

  if (strlen(basestr) > 0)
  {
    if (!addstr(&cmdstr, (char*)"G", &addlen)) goto error;
  }

  return true;

error:
  DBGOUT((F("Pattern string longer than: %d"), maxlen));
  return false;
}

bool PixelNutEngine::updateEffects(void)
{
  bool doshow = (timePrevUpdate == 0);

  uint32_t time = pixelNutSupport.getMsecs();
  bool rollover = (timePrevUpdate > time);
  timePrevUpdate = time;

  CheckAutoTrigger(rollover);

  // first have any redraw effects that are ready draw into its own buffers...

  PluginTrack *pTrack = pluginTracks;
  for (int i = 0; i <= indexTrackStack; ++i, ++pTrack) // for each plugin that can redraw
  {
    if (i > indexTrackEnable) break; // at top of active layers now

    PluginLayer *pLayer = &pluginLayers[pTrack->layer];

      // don't show if layer is disabled or not triggered yet
    if (pLayer->disable || !pLayer->trigActive) continue;

    // update the time if it's rolled over, then check if time to draw
    if (rollover) pTrack->msTimeRedraw = timePrevUpdate;
    if (pTrack->msTimeRedraw > timePrevUpdate) continue;

    //DBGOUT((F("redraw buffer: track=%d msecs=%lu"), i, pTrack->msTimeRedraw));

    short pixCount = 0;
    short degreeHue = 0;
    byte pcentWhite = 0;

    // prevent predraw effect from overwriting properties if in extern mode
    if (externPropMode)
    {
      pixCount = pTrack->draw.pixCount;
      degreeHue = pTrack->draw.degreeHue;
      pcentWhite = pTrack->draw.pcentWhite;
    }

    pDrawPixels = NULL; // prevent drawing by predraw effects

    // call all filter effects associated with this track if triggered
    for (int j = 0; j <= indexLayerStack; ++j)
      if ((pluginLayers[j].track == i)  &&
          !pluginLayers[j].redraw       &&
           pluginLayers[j].trigActive)
           pluginLayers[j].pPlugin->nextstep(this, &pTrack->draw);

    if (externPropMode) RestorePropVals(pTrack, pixCount, degreeHue, pcentWhite);

    // now the main drawing effect is executed for this track
    pDrawPixels = pTrack->pRedrawBuff; // switch to drawing buffer
    pLayer->pPlugin->nextstep(this, &pTrack->draw);
    pDrawPixels = pDisplayPixels; // restore to default (display buffer)

    short addtime = pTrack->draw.msecsDelay + delayOffset;
    //DBGOUT((F("delay=%d.%d.%d"), pTrack->draw.msecsDelay, delayOffset, addtime));
    if (addtime <= 0) addtime = 1; // must advance at least by 1 each time
    pTrack->msTimeRedraw = timePrevUpdate + addtime;

    doshow = true;
  }

  if (doshow)
  {
    // merge all buffers whether just redrawn or not if anyone of them changed
    memset(pDisplayPixels, 0, (numPixels*3)); // must clear display buffer first

    pTrack = pluginTracks;
    for (int i = 0; i <= indexTrackStack; ++i, ++pTrack) // for each plugin that can redraw
    {
      if (i > indexTrackEnable) break; // at top of active layers now

      PluginLayer *pLayer = &pluginLayers[pTrack->layer];

      // don't show if layer is disabled or not triggered yet,
      // but do draw if just not updated from above
      if (pLayer->disable || !pLayer->trigActive) continue;

      short pixlast = numPixels-1;
      short pixstart = firstPixel + pTrack->draw.pixStart;
      //DBGOUT((F("%d PixStart: %d == %d+%d"), pTrack->draw.goUpwards, pixstart, firstPixel, pTrack->draw.pixStart));
      if (pixstart > pixlast) pixstart -= (pixlast+1);

      short pixend = pixstart + pTrack->draw.pixLen - 1;
      //DBGOUT((F("%d PixEnd:  %d == %d+%d-1"), pTrack->draw.goUpwards, pixend, pixstart, pTrack->draw.pixLen));
      if (pixend > pixlast) pixend -= (pixlast+1);

      short pix = (pTrack->draw.goUpwards ? pixstart : pixend);
      short x = pix * 3;
      short y = pTrack->draw.pixStart * 3;

      /*
      byte *p = pTrack->pRedrawBuff;
      DBGOUT((F("Input pixels:")));
      for (int i = 0; i < numPixels; ++i)
        DBGOUT((F("  %d.%d.%d"), *p++, *p++, *p++));
      */
      while(true)
      {
        //DBGOUT((F(">> start.end=%d.%d pix=%d x=%d y=%d"), pixstart, pixend, pix, x, y));

        if (!pTrack->draw.pixOverwrite)
        {
          // combine contents of buffer window with actual pixel array
          pDisplayPixels[x+0] |= pTrack->pRedrawBuff[y+0];
          pDisplayPixels[x+1] |= pTrack->pRedrawBuff[y+1];
          pDisplayPixels[x+2] |= pTrack->pRedrawBuff[y+2];
        }
        else if ((pTrack->pRedrawBuff[y+0] != 0) ||
                 (pTrack->pRedrawBuff[y+1] != 0) ||
                 (pTrack->pRedrawBuff[y+2] != 0))
        {
          pDisplayPixels[x+0] = pTrack->pRedrawBuff[y+0];
          pDisplayPixels[x+1] = pTrack->pRedrawBuff[y+1];
          pDisplayPixels[x+2] = pTrack->pRedrawBuff[y+2];
        }

        if (pTrack->draw.goUpwards)
        {
          if (pix == pixend) break;

          if (pix >= pixlast) // wrap around to start of strip
          {
            pix = x = 0;
          }
          else
          {
            ++pix;
            x += 3;
          }
        }
        else // going backwards
        {
          if (pix == pixstart) break;
  
          if (pix <= 0) // wrap around to end of strip
          {
            pix = pixlast;
            x = (pixlast * 3);
          }
          else
          {
            --pix;
            x -= 3;
          }
        }

        if (y >= (pixlast*3)) y = 0;
        else y += 3;
      }
    }

    /*
    byte *p = pDisplayPixels;
    for (int i = 0; i < numPixels; ++i)
      DBGOUT((F("%d.%d.%d"), *p++, *p++, *p++));
    */
  }

  return doshow;
}
