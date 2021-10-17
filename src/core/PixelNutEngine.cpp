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
// initialize class variables, allocate memory for layer/track stacks and pixel buffer
////////////////////////////////////////////////////////////////////////////////////////////////////

bool PixelNutEngine::init(uint16_t num_pixels, byte num_bytes,
                          byte num_layers, byte num_tracks,
                          uint16_t first_pixel, bool backwards)
{
  pixelBytes = num_pixels * num_bytes;

  pluginLayers  = (PluginLayer*)malloc(num_layers * sizeof(PluginLayer));
  pluginTracks  = (PluginTrack*)malloc(num_tracks * sizeof(PluginTrack));
  pTrackBuffers = (byte *)malloc(num_tracks * pixelBytes);

  pDisplayPixels = (byte*)malloc(pixelBytes);
  memset(pDisplayPixels, 0, pixelBytes);

  if ((pluginLayers  == NULL) || (pluginTracks   == NULL) ||
      (pTrackBuffers == NULL) || (pDisplayPixels == NULL))
    return false;

  numPixels         = num_pixels;
  numBytesPerPixel  = num_bytes;
  firstPixel        = first_pixel;
  goBackwards       = backwards;

  maxPluginLayers = (short)num_layers;
  maxPluginTracks = (short)num_tracks;

  pDrawPixels = pDisplayPixels;
  return true;
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
static short GetNumValue(char *str, int maxval)
{
  if ((str == NULL) || !isdigit(*str)) return -1;
  int newval = atoi(str);
  if (newval > maxval) return -1;
  if (newval < 0) return -1;
  return newval;
}

// clips values to range 0-'maxval'
// returns 'curval' if no value is specified
static short GetNumValue(char *str, int curval, int maxval) // FIXME: use signed values
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
  }

  indexTrackEnable = -1;
  indexLayerStack  = -1;
  indexTrackStack  = -1;

  // clear all pixels too
  memset(pDisplayPixels, 0, pixelBytes);
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
    memset(pTrack, 0, sizeof(PluginTrack)); // set all to 0

    pTrack->layer = indexLayerStack;

    // initialize track drawing properties to default values
    PixelNutSupport::DrawProps *pProps = &pTrack->draw;
    pTrack->draw.pixLen = numPixels; // set initial window
    pProps->pixCount = pixelNutSupport.mapValue(DEF_PCENTCOUNT, 0, MAX_PERCENTAGE, 1, numPixels);

    SETVAL_IF_NONZERO(pProps->pcentBright, DEF_PCENTBRIGHT);
    SETVAL_IF_NONZERO(pProps->msecsDelay,  DEF_DELAYMSECS);
    SETVAL_IF_NONZERO(pProps->degreeHue,   DEF_DEGREESHUE);
    SETVAL_IF_NONZERO(pProps->pcentWhite,  DEF_PCENTWHITE);
    SETVAL_IF_NONZERO(pProps->goBackwards, DEF_BACKWARDS);
    SETVAL_IF_NONZERO(pProps->pixOrValues, DEF_PIXORVALS);

    pixelNutSupport.makeColorVals(pProps); // create RGB values
  }

  PluginLayer *pLayer = &pluginLayers[indexLayerStack];
  pLayer->pPlugin = pPlugin;
  pLayer->iplugin = iplugin;
  pLayer->redraw  = dodraw;
  pLayer->track   = indexTrackStack;

  SETVAL_IF_NONZERO(pLayer->trigForce,     DEF_FORCEVAL);
  SETVAL_IF_NONZERO(pLayer->trigRepCount,  DEF_TRIG_FOREVER);
  SETVAL_IF_NONZERO(pLayer->trigRepOffset, DEF_TRIG_OFFSET);
  SETVAL_IF_NONZERO(pLayer->trigRepRange,  DEF_TRIG_RANGE);
  // all other parameters have been initialized to 0 with memset

  DBGOUT((F("Append plugin: #%d type=0x%02X redraw=%d track=%d layer=%d"),
          iplugin, ptype, dodraw, indexTrackStack, indexLayerStack));

  // begin new plugin, but will not be drawn until triggered
  pPlugin->begin(indexLayerStack, numPixels);

  if (dodraw)
  {
    // must clear buffer to remove current drawn pixels
    memset(&pTrackBuffers[pLayer->track * pixelBytes], 0, pixelBytes);
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
  memset(&pTrackBuffers[pLayer->track * pixelBytes], 0, pixelBytes);

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

    // must clear buffer to remove current drawn pixels
    memset(&pTrackBuffers[pLayer->track * pixelBytes], 0, pixelBytes);
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
  // prevent drawing if not filter effect
  pDrawPixels = (pLayer->redraw ? &pTrackBuffers[track * pixelBytes] : NULL);
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
    if (pluginLayers[i].track > indexTrackEnable) break; // not enabled yet

    // just always reset trigger time after rollover event
    if (rollover && (pluginLayers[i].trigType & TrigTypeBit_Repeating))
      pluginLayers[i].trigTimeMsecs = timePrevUpdate;

    if ((pluginLayers[i].trigType & TrigTypeBit_Repeating)               && // auto-triggering set
        (pluginLayers[i].trigDnCounter || !pluginLayers[i].trigRepCount) && // have count (or infinite)
        (pluginLayers[i].trigTimeMsecs <= timePrevUpdate))                  // and time has expired
    {
      DBGOUT((F("RepeatTrigger: prevtime=%lu msecs=%lu offset=%u range=%d counts=%d:%d"),
                timePrevUpdate, pluginLayers[i].trigTimeMsecs,
                pluginLayers[i].trigRepOffset, pluginLayers[i].trigRepRange,
                pluginLayers[i].trigRepCount, pluginLayers[i].trigDnCounter));

      short force = ((pluginLayers[i].trigForce >= 0) ? 
                      pluginLayers[i].trigForce : random(0, MAX_FORCE_VALUE+1));

      triggerLayer(i, force);

      pluginLayers[i].trigTimeMsecs = timePrevUpdate +
          (1000 * random(pluginLayers[i].trigRepOffset,
                        (pluginLayers[i].trigRepOffset +
                        pluginLayers[i].trigRepRange+1)));

      if (pluginLayers[i].trigDnCounter > 0) --pluginLayers[i].trigDnCounter;
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
      triggerLayer(i, force);
  }
}

// internal: called from plugins
void PixelNutEngine::triggerForce(byte layer, short force, PixelNutSupport::DrawProps *pdraw)
{
  for (int i = 0; i <= indexLayerStack; ++i)
  {
    if (!pluginLayers[i].disable &&
        (pluginLayers[i].trigLayer == layer) &&
        (pluginLayers[i].trigType & TrigTypeBit_Internal))
      triggerLayer(i, force);
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
          pdraw->pixStart = (uint16_t)GetNumValue(cmd+1, 0, numPixels-1);
          DBGOUT((F(">> Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'Y': // number of pixels in the track by pixel index
        {
          pdraw->pixLen = (uint16_t)GetNumValue(cmd+1, 0, numPixels);
          DBGOUT((F(">> Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'J': // offset into output display of the track by percent
        {
          uint16_t pcent = (uint16_t)GetNumValue(cmd+1, 0, MAX_PERCENTAGE);
          pdraw->pixStart = pixelNutSupport.mapValue(pcent, 0, MAX_PERCENTAGE, 0, numPixels-1);
          DBGOUT((F(">> Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'K': // number of pixels in the track by percent
        {
          uint16_t pcent = (uint16_t)GetNumValue(cmd+1, 0, MAX_PERCENTAGE);
          pdraw->pixLen = pixelNutSupport.mapValue(pcent, 0, MAX_PERCENTAGE, 1, numPixels);
          DBGOUT((F(">> Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'B': // percent brightness property ("B" sets default value)
        {
          pdraw->pcentBright = (byte)GetNumValue(cmd+1, DEF_PCENTBRIGHT, MAX_PERCENTAGE);
          pixelNutSupport.makeColorVals(pdraw);
          break;
        }
        case 'D': // drawing delay ("D" sets default value)
        {
          pdraw->msecsDelay = (byte)GetNumValue(cmd+1, DEF_DELAYMSECS, MAX_DELAY_VALUE);
          break;
        }
        case 'H': // color Hue degrees property ("H" sets default value)
        {
          pdraw->degreeHue = (uint16_t)GetNumValue(cmd+1, DEF_DEGREESHUE, MAX_DEGREES_HUE);
          pixelNutSupport.makeColorVals(pdraw);
          break;
        }
        case 'W': // percent Whiteness property ("W" sets default value)
        {
          pdraw->pcentWhite = (byte)GetNumValue(cmd+1, DEF_PCENTWHITE, MAX_PERCENTAGE);
          pixelNutSupport.makeColorVals(pdraw);
          break;
        }
        case 'C': // pixel count property ("C" sets default value)
        {
          uint16_t percent = (uint16_t)GetNumValue(cmd+1, DEF_PCENTCOUNT, MAX_PERCENTAGE);
          pdraw->pixCount = pixelNutSupport.mapValue(percent, 0, MAX_PERCENTAGE, 1, numPixels);
          DBGOUT((F("PixCount: %d%% => %d"), percent, pdraw->pixCount));
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
        case 'U': // go backwards ("U" for not default, else sets value)
        {
          pdraw->goBackwards = GetBoolValue(cmd+1, !DEF_BACKWARDS);
          break;
        }
        case 'V': // OR's pixels Values ("V" for not default, else sets value)
        {
          pdraw->pixOrValues = GetBoolValue(cmd+1, !DEF_PIXORVALS);
          break;
        }
        case 'F': // force value to be used by trigger ("F" means random force)
        {
          if (isdigit(*(cmd+1))) // there is a value after "F" (clip to 0-MAX_FORCE_VALUE)
               pluginLayers[curlayer].trigForce = GetNumValue(cmd+1, 0, MAX_FORCE_VALUE);
          else pluginLayers[curlayer].trigForce = -1; // get random value each time
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
        case 'I': // external triggering enable ('I0' to disable)
        {
          if (GetBoolValue(cmd+1, true))
               pluginLayers[curlayer].trigType |=  TrigTypeBit_External;
          else pluginLayers[curlayer].trigType &= ~TrigTypeBit_External;
          break;
        }
        case 'A': // assign effect layer as trigger source ("A" to disable)
        {
          if (isdigit(*(cmd+1))) // there is a value after "A"
          {
            pluginLayers[curlayer].trigType |= TrigTypeBit_Internal;
            pluginLayers[curlayer].trigLayer = (byte)GetNumValue(cmd+1, MAX_BYTE_VALUE, MAX_BYTE_VALUE);
            DBGOUT((F("Triggering assigned to layer %d"), pluginLayers[curlayer].trigLayer));
          }
          else pluginLayers[curlayer].trigType &= ~TrigTypeBit_Internal;
          break;
        }
        case 'R': // sets repeat trigger ("R0" to disable, "R" for forever count, else "R<count>")
        {
          bool enable = true;

          if (isdigit(*(cmd+1))) // there is a value after "R"
          {
            uint16_t value = (uint16_t)GetNumValue(cmd+1, 0, MAX_WORD_VALUE);
            if (value == 0) enable = false;
            else pluginLayers[curlayer].trigRepCount = value;
          }

          if (enable)
          {
            pluginLayers[curlayer].trigType |= TrigTypeBit_Repeating;
            pluginLayers[curlayer].trigDnCounter = pluginLayers[curlayer].trigRepCount;

            pluginLayers[curlayer].trigTimeMsecs = pixelNutSupport.getMsecs() +
                (1000 * random(pluginLayers[curlayer].trigRepOffset,
                              (pluginLayers[curlayer].trigRepOffset + pluginLayers[curlayer].trigRepRange+1)));

            DBGOUT((F("RepeatTrigger: layer=%d offset=%u range=%d count=%d force=%d"), curlayer,
                      pluginLayers[curlayer].trigRepOffset, pluginLayers[curlayer].trigRepRange,
                      pluginLayers[curlayer].trigRepCount, pluginLayers[curlayer].trigForce));
          }
          else pluginLayers[curlayer].trigType &= ~TrigTypeBit_Repeating;
          break;
        }
        case 'O': // repeat trigger offset time ("O" sets default value)
        {
          pluginLayers[curlayer].trigRepOffset = (uint16_t)GetNumValue(cmd+1, DEF_TRIG_OFFSET, MAX_WORD_VALUE);
          break;
        }
        case 'N': // range of trigger time ("N" sets default value)
        {
          pluginLayers[curlayer].trigRepRange = (uint16_t)GetNumValue(cmd+1, DEF_TRIG_RANGE, MAX_WORD_VALUE);
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
  char *basestr = cmdstr;
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
        int pcent = pixelNutSupport.mapValue(pdraw->pixStart, 0, numPixels-1, 0, MAX_PERCENTAGE);
        sprintf(str, "J%d ", pcent);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->pixLen != numPixels)
      {
        int pcent = pixelNutSupport.mapValue(pdraw->pixLen, 1, numPixels, 0, MAX_PERCENTAGE);
        sprintf(str, "K%d ", pcent);
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
      if (pcent != DEF_PCENTCOUNT)
      {
        sprintf(str, "C%d ", pcent);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pTrack->ctrlBits != 0)
      {
        sprintf(str, "Q%d ", pTrack->ctrlBits);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->goBackwards != DEF_BACKWARDS)
      {
        sprintf(str, "U ");
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->pixOrValues != DEF_PIXORVALS)
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

    if (pLayer->trigType & TrigTypeBit_AtStart)
    {
      sprintf(str, "T ");
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }

    if (pLayer->trigType & TrigTypeBit_External)
    {
      sprintf(str, "I ");
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }

    if (pLayer->trigType & TrigTypeBit_Internal)
    {
      sprintf(str, "A%d ", pLayer->trigLayer);
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }

    if (pLayer->trigType & TrigTypeBit_Repeating)
    {
      if (pLayer->trigRepCount == DEF_TRIG_FOREVER)
           sprintf(str, "R ");
      else sprintf(str, "R%d ", pLayer->trigRepCount);
      if (!addstr(&cmdstr, str, &addlen)) goto error;

      if (pLayer->trigRepOffset != DEF_TRIG_OFFSET)
      {
        sprintf(str, "O%d ", pLayer->trigRepOffset);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pLayer->trigRepRange != DEF_TRIG_RANGE)
      {
        sprintf(str, "N%d ", pLayer->trigRepRange);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }
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

  RepeatTriger(rollover);

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
    pDrawPixels = &pTrackBuffers[i * pixelBytes]; // switch to drawing buffer
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
    memset(pDisplayPixels, 0, pixelBytes); // must clear display buffer first

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
      //DBGOUT((F("%d PixStart: %d == %d+%d"), pTrack->draw.goBackwards, pixstart, firstPixel, pTrack->draw.pixStart));
      if (pixstart > pixlast) pixstart -= (pixlast+1);

      short pixend = pixstart + pTrack->draw.pixLen - 1;
      //DBGOUT((F("%d PixEnd:  %d == %d+%d-1"), pTrack->draw.goBackwards, pixend, pixstart, pTrack->draw.pixLen));
      if (pixend > pixlast) pixend -= (pixlast+1);

      short pix = (pTrack->draw.goBackwards ? pixend : pixstart);
      short x = pix * numBytesPerPixel;
      short y = pTrack->draw.pixStart * numBytesPerPixel;

      byte *ppix = &pTrackBuffers[i * pixelBytes];
      /*
      DBGOUT((F("Input pixels:")));
      for (int i = 0; i < numPixels; ++i)
        DBGOUT((F("  %d.%d.%d"), *p++, *p++, *p++));
      */
      while(true)
      {
        //DBGOUT((F(">> start.end=%d.%d pix=%d x=%d y=%d"), pixstart, pixend, pix, x, y));

        if (pTrack->draw.pixOrValues)
        {
          // combine contents of buffer window with actual pixel array
          pDisplayPixels[x+0] |= ppix[y+0];
          pDisplayPixels[x+1] |= ppix[y+1];
          pDisplayPixels[x+2] |= ppix[y+2];
        }
        else if ((ppix[y+0] != 0) ||
                 (ppix[y+1] != 0) ||
                 (ppix[y+2] != 0))
        {
          pDisplayPixels[x+0] = ppix[y+0];
          pDisplayPixels[x+1] = ppix[y+1];
          pDisplayPixels[x+2] = ppix[y+2];
        }

        if (!pTrack->draw.goBackwards)
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
            x = (pixlast * numBytesPerPixel);
          }
          else
          {
            --pix;
            x -= 3;
          }
        }

        if (y >= (pixlast * numBytesPerPixel)) y = 0;
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
