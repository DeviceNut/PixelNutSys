// PixelNut Engine Class Implementation
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file (must also set in main.h)

#include "core.h"
#include "PixelNutPlugin.h"
#include "PixelNutEngine.h"

extern PluginFactory *pPluginFactory; // use externally declared pointer to instance

////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor: initialize class variables, allocate memory for layer/track stacks
////////////////////////////////////////////////////////////////////////////////////////////////////

PixelNutEngine::PixelNutEngine(byte *ptr_pixels, uint16_t num_pixels,
                               uint16_t first_pixel, bool goupwards,
                               short num_layers, short num_tracks)
{
  // NOTE: cannot call DBGOUT here if statically constructed

  pDisplayPixels  = ptr_pixels;
  numPixels       = num_pixels;
  firstPixel      = first_pixel;
  goUpwards       = goupwards;

  maxPluginLayers = num_layers;
  maxPluginTracks = num_tracks;

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

// set or toggle value according to char at 'str'
static bool GetBoolValue(char *str, bool curval)
{
  if (*str == '0') return false;
  if (*str == '1') return true;
  return !curval;
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
static int GetNumValue(char *str, int curval, int maxval)
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
  DBGOUT((F("Clear stack: layer=%d track=%d"), indexLayerStack, indexTrackStack));

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
PixelNutEngine::Status PixelNutEngine::NewPluginLayer(int plugin)
{
  // check if can add another layer to the stack
  if ((indexLayerStack+1) >= maxPluginLayers)
  {
    DBGOUT((F("Cannot add another layer: max=%d"), (indexLayerStack+1)));
    return Status_Error_Memory;
  }

  PixelNutPlugin *pPlugin = pPluginFactory->makePlugin(plugin);
  if (pPlugin == NULL) return Status_Error_BadVal;

  // determine if must allocate buffer for track, or is a filter plugin
  bool newtrack = (pPlugin->gettype() & PLUGIN_TYPE_REDRAW);

  DBGOUT((F("NewTrack=%d Layer=%d Track=%d"), newtrack, indexLayerStack, indexTrackStack));

  // check if:
  // a filter plugin and there is at least one redraw plugin, or
  // a redraw plugin and cannot add another track to the stack
  if ((!newtrack && (indexTrackStack < 0)) ||
      ( newtrack && ((indexTrackStack+1) >= maxPluginTracks)))
  {
    delete pPlugin;

    if (newtrack)
    {
      DBGOUT((F("Cannot add another track: max=%d"), (indexTrackStack+1)));
      return Status_Error_Memory;
    }
    else
    {
      DBGOUT((F("First plugin must be a track: #%d"), plugin));
      return Status_Error_BadCmd;
    }
  }

  ++indexLayerStack; // stack another effect layer
  memset(&pluginLayers[indexLayerStack], 0, sizeof(PluginLayer));

  if (newtrack)
  {
    ++indexTrackStack; // create another effect track
    PluginTrack *pTrack = &pluginTracks[indexTrackStack];

    pTrack->layer     = indexLayerStack;
    pTrack->ctrlBits  = 0;  // allow overwriting by default
    pTrack->disable   = 0;

    // initialize track drawing properties: some must be set with user commands
    memset(&pTrack->draw, 0, sizeof(PixelNutSupport::DrawProps));
    pTrack->draw.pixLen        = numPixels;        // set initial window (start was memset)
    pTrack->draw.pcentBright   = MAX_PERCENTAGE;  // start off with max brightness
    pTrack->draw.pixCount      = 1;               // default count is 1
    // default hue is 0(red), white is 0, delay is 0
    pTrack->draw.goUpwards     = goUpwards;       // default direction on strip
    pTrack->draw.orPixelValues = true;            // OR with other tracks
    pixelNutSupport.makeColorVals(&pTrack->draw); // create RGB values
  }

  PluginLayer *pLayer = &pluginLayers[indexLayerStack];
  pLayer->track         = indexTrackStack;
  pLayer->pPlugin       = pPlugin;
  pLayer->trigCount     = -1; // forever
  pLayer->trigDelayMin  = 1;  // 1 sec min
  pLayer->trigSource    = MAX_BYTE_VALUE; // disabled
  pLayer->trigForce     = curForce; // used currently set force for default
  // Note: all other trigger parameters are initialized to 0

  DBGOUT((F("Added plugin #%d: type=0x%02X layer=%d track=%d"),
        plugin, pPlugin->gettype(), indexLayerStack, indexTrackStack));

  // begin new plugin, but will not be drawn until triggered
  pPlugin->begin(indexLayerStack, numPixels);

  if (newtrack) // wait to do this until after any memory allocation in plugin
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Trigger force handling routines
////////////////////////////////////////////////////////////////////////////////////////////////////

void PixelNutEngine::triggerLayer(byte layer, short force)
{
  PluginLayer *pLayer = &pluginLayers[layer];
  int track = pLayer->track;
  PluginTrack *pTrack = &pluginTracks[track];

  bool predraw = !(pLayer->pPlugin->gettype() & PLUGIN_TYPE_REDRAW);

  DBGOUT((F("Trigger: layer=%d track=%d(L%d) force=%d"), layer, track, pTrack->layer, force));

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

  byte *dptr = pDrawPixels;
  pDrawPixels = (predraw ? NULL : pTrack->pRedrawBuff); // prevent drawing if not drawing effect
  pLayer->pPlugin->trigger(this, &pTrack->draw, force);
  pDrawPixels = dptr; // restore to the previous value

  if (externPropMode) RestorePropVals(pTrack, pixCount, degreeHue, pcentWhite);

  // if this is the drawing effect for the track then redraw immediately
  if (!predraw) pTrack->msTimeRedraw = pixelNutSupport.getMsecs();

  pLayer->trigActive = true; // layer has been triggered now
}

// internal: check for any automatic triggering
void PixelNutEngine::CheckAutoTrigger(bool rollover)
{
  for (int i = 0; i <= indexLayerStack; ++i) // for each plugin layer
  {
    if (pluginLayers[i].track > indexTrackEnable) break; // not enabled yet

    // just always reset trigger time after rollover event
    if (rollover && (pluginLayers[i].trigTimeMsecs > 0))
      pluginLayers[i].trigTimeMsecs = timePrevUpdate;

    if (pluginLayers[i].trigActive &&                       // triggering is active
        pluginLayers[i].trigCount  &&                       // have count (or infinite)
        (pluginLayers[i].trigTimeMsecs > 0) &&              // auto-triggering set
        (pluginLayers[i].trigTimeMsecs <= timePrevUpdate))  // and time has expired
    {
      DBGOUT((F("AutoTrigger: prevtime=%lu msecs=%lu delay=%u+%u count=%d"),
                timePrevUpdate, pluginLayers[i].trigTimeMsecs,
                pluginLayers[i].trigDelayMin, pluginLayers[i].trigDelayRange,
                pluginLayers[i].trigCount));

      short force = ((pluginLayers[i].trigForce >= 0) ? pluginLayers[i].trigForce : random(0, MAX_FORCE_VALUE+1));

      triggerLayer(i, force);

      pluginLayers[i].trigTimeMsecs = timePrevUpdate +
          (1000 * random(pluginLayers[i].trigDelayMin,
                        (pluginLayers[i].trigDelayMin + pluginLayers[i].trigDelayRange+1)));

      if (pluginLayers[i].trigCount > 0) --pluginLayers[i].trigCount;
    }
  }
}

// external: cause trigger if enabled in track
void PixelNutEngine::triggerForce(short force)
{
  curForce = force; // sets default for new patterns

  for (int i = 0; i <= indexLayerStack; ++i)
    if (pluginLayers[i].trigExtern)
      triggerLayer(i, force);
}

// internal: called from plugins
void PixelNutEngine::triggerForce(byte layer, short force, PixelNutSupport::DrawProps *pdraw)
{
  for (int i = 0; i <= indexLayerStack; ++i)
    if (layer == pluginLayers[i].trigSource)
      triggerLayer(i, force);
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
    if (pTrack->disable) continue;

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
    if (pTrack->disable) continue;

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
void PixelNutEngine::RestorePropVals(PluginTrack *pTrack, uint16_t pixCount, uint16_t degreeHue, byte pcentWhite)
{
  if (pTrack->disable) return;

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
// Uses all alpha characters except: R,S
////////////////////////////////////////////////////////////////////////////////////////////////////

PixelNutEngine::Status PixelNutEngine::execCmdStr(char *cmdstr)
{
  Status status = Status_Success;

  int curlayer = indexLayerStack;
  int curtrack = indexTrackStack;

  for (int i = 0; cmdstr[i]; ++i) // convert to upper case
    cmdstr[i] = toupper(cmdstr[i]);

  char *cmd = strtok(cmdstr, " "); // separate options by spaces

  if (cmd == NULL) return Status_Success; // ignore empty line
  do
  {
    PixelNutSupport::DrawProps *pdraw = NULL;
    if (curtrack >= 0) pdraw = &pluginTracks[curtrack].draw;

    DBGOUT((F(">> Cmd=%s len=%d curtrack=%d"), cmd, strlen(cmd), curtrack));

    if (cmd[0] == 'E') // add a plugin Effect to the stack ("E" is an error)
    {
      int plugin = GetNumValue(cmd+1, MAX_PLUGIN_VALUE); // returns -1 if not within range
      if (plugin >= 0)
      {
        status = NewPluginLayer(plugin);
        if (status == Status_Success)
        {
          curtrack = indexTrackStack;
          curlayer = indexLayerStack;
        }
        else { DBGOUT((F("Cannot add plugin #%d: layer=%d track=%d"), plugin, indexLayerStack, indexTrackStack)); }
      }
      else status = Status_Error_BadVal;
    }
    else if (cmd[0] == 'P') // Pop one or more plugins from the stack ('P' is same as 'P0': pop all)
    {
      clearStack();
      curlayer = curtrack = -1; // must reset these after clear
      timePrevUpdate = 0; // redisplay pixels after being cleared
    }
    else if (cmd[0] == 'M') // set plugin layer to Modify ('M' uses top of stack)
    {
      curlayer = GetNumValue(cmd+1, indexLayerStack); // returns -1 if not within range
      if (curlayer < 0) curlayer = indexLayerStack;
      curtrack = pluginLayers[curlayer].track;
    }
    else if (pdraw != NULL)
    {
      switch (cmd[0])
      {
        case 'J': // sets offset into output display of the current track by percent
        {
          pdraw->pixStart = (GetNumValue(cmd+1, 0, MAX_PERCENTAGE) * (numPixels-1)) / MAX_PERCENTAGE;
          DBGOUT((F(">> Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'K': // sets number of pixels in the current track by percent
        {
          pdraw->pixLen = ((GetNumValue(cmd+1, 0, MAX_PERCENTAGE) * (numPixels-1)) / MAX_PERCENTAGE) + 1;
          DBGOUT((F(">> Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'U': // set the pixel direction in the current track properties ("U1" is default(up), "U" toggles value)
        {
          pdraw->goUpwards = GetBoolValue(cmd+1, pdraw->goUpwards);
          break;
        }
        case 'V': // set whether to oVerwrite pixels in the current track properties ("V0" is default(OR), "V" toggles value)
        {
          pdraw->orPixelValues = !GetBoolValue(cmd+1, !pdraw->orPixelValues);
          break;
        }
        case 'H': // set the color Hue in the current track properties ("H" has no effect)
        {
          pdraw->degreeHue = GetNumValue(cmd+1, pdraw->degreeHue, MAX_DEGREES_HUE);
          pixelNutSupport.makeColorVals(pdraw);
          break;
        }
        case 'W': // set the Whiteness in the current track properties ("W" has no effect)
        {
          pdraw->pcentWhite = GetNumValue(cmd+1, pdraw->pcentWhite, MAX_PERCENTAGE);
          pixelNutSupport.makeColorVals(pdraw);
          break;
        }
        case 'B': // set the Brightness in the current track properties ("B" has no effect)
        {
          pdraw->pcentBright = GetNumValue(cmd+1, pdraw->pcentBright, MAX_PERCENTAGE);
          pixelNutSupport.makeColorVals(pdraw);
          break;
        }
        case 'C': // set the pixel Count in the current track properties ("C" has no effect)
        {
          short curvalue = ((pdraw->pixCount * MAX_PERCENTAGE) / numPixels);
          short percent = GetNumValue(cmd+1, curvalue, MAX_PERCENTAGE);
          DBGOUT((F("CurCount: %d==%d%% numPixels=%d"), pdraw->pixCount, curvalue, numPixels));

          // map value into a pixel count, dependent on the actual number of pixels
          pdraw->pixCount = pixelNutSupport.mapValue(percent, 0, MAX_PERCENTAGE, 1, numPixels);
          DBGOUT((F("PixCount: %d==%d%%"), pdraw->pixCount, percent));
          break;
        }
        case 'D': // set the delay in the current track properties ("D" has no effect)
        {
          pdraw->msecsDelay = GetNumValue(cmd+1, pdraw->msecsDelay, MAX_DELAY_VALUE);
          break;
        }
        case 'Q': // set extern control bits ("Q" has no effect)
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
        case 'I': // set external triggering enable ('I0' to disable, "I" is same as "I1")
        {
          if (isdigit(*(cmd+1))) // there is a value after "I"
               pluginLayers[curlayer].trigExtern = GetBoolValue(cmd+1, false);
          else pluginLayers[curlayer].trigExtern = true;
          break;
        }
        case 'A': // Assign effect layer as trigger source for current plugin layer ("A" is same as "A0", "A255" disables)
        {
          pluginLayers[curlayer].trigSource = GetNumValue(cmd+1, 0, MAX_BYTE_VALUE); // clip to 0-MAX_BYTE_VALUE
          DBGOUT((F("Triggering assigned to layer %d"), pluginLayers[curlayer].trigSource));
          break;
        }
        case 'F': // set Force value to be used by trigger ("F" causes random force to be used)
        {
          if (isdigit(*(cmd+1))) // there is a value after "F"
               pluginLayers[curlayer].trigForce = GetNumValue(cmd+1, 0, MAX_FORCE_VALUE); // clip to 0-MAX_FORCE_VALUE
          else pluginLayers[curlayer].trigForce = -1; // get random value each time
          break;
        }
        case 'N': // Auto trigger counter ("N" or "N0" means forever, same as not specifying at all)
        {         // (this count does NOT include the initial trigger from the "T" command)
          pluginLayers[curlayer].trigCount = GetNumValue(cmd+1, 0, MAX_WORD_VALUE); // clip to 0-MAX_WORD_VALUE
          if (!pluginLayers[curlayer].trigCount) pluginLayers[curlayer].trigCount = -1;
          break;
        }
        case 'O': // sets minimum auto-triggering time ("O", "O0", "O1" all get set to default(1sec))
        {
          uint16_t min = GetNumValue(cmd+1, 1, MAX_WORD_VALUE); // clip to 0-MAX_WORD_VALUE
          pluginLayers[curlayer].trigDelayMin = min ? min : 1;
          break;
        }
        case 'T': // Trigger the current plugin layer, either once ("T") or with timer ("T<n>")
        {
          short force = pluginLayers[curlayer].trigForce;
          if (force < 0) force = random(0, MAX_FORCE_VALUE+1);

          if (isdigit(*(cmd+1))) // there is a value after "T"
          {
            pluginLayers[curlayer].trigDelayRange = GetNumValue(cmd+1, 0, MAX_WORD_VALUE); // clip to 0-MAX_WORD_VALUE
            pluginLayers[curlayer].trigTimeMsecs = pixelNutSupport.getMsecs() +
                (1000 * random(pluginLayers[curlayer].trigDelayMin,
                              (pluginLayers[curlayer].trigDelayMin + pluginLayers[curlayer].trigDelayRange+1)));

            DBGOUT((F("AutoTriggerSet: layer=%d delay=%u+%u count=%d force=%d"), curlayer,
                      pluginLayers[curlayer].trigDelayMin, pluginLayers[curlayer].trigDelayRange,                          
                      pluginLayers[curlayer].trigCount, force));
          }

          triggerLayer(curlayer, force); // always trigger immediately
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

    if (!(pluginLayers[pTrack->layer].pPlugin->gettype() & PLUGIN_TYPE_REDRAW))
      continue;

    if (rollover) pTrack->msTimeRedraw = timePrevUpdate;

    //DBGOUT((F("redraw buffer: track=%d layer=%d type=0x%04X"), i, pTrack->layer,
    //        pluginLayers[pTrack->layer].pPlugin->gettype()));

    // don't draw if the layer hasn't been triggered yet, or it's not time yet
    if (!pluginLayers[pTrack->layer].trigActive) continue;
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

    // call all of the predraw effects associated with this track
    for (int j = 0; j <= indexLayerStack; ++j)
      if ((pluginLayers[j].track == i) && pluginLayers[j].trigActive &&
          !(pluginLayers[j].pPlugin->gettype() & PLUGIN_TYPE_REDRAW))
            pluginLayers[j].pPlugin->nextstep(this, &pTrack->draw);

    if (externPropMode) RestorePropVals(pTrack, pixCount, degreeHue, pcentWhite);

    // now the main drawing effect is executed for this track
    pDrawPixels = pTrack->pRedrawBuff; // switch to drawing buffer
    pluginLayers[pTrack->layer].pPlugin->nextstep(this, &pTrack->draw);
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
    memset(pDisplayPixels, 0, (numPixels*3)); // must clear output buffer first

    pTrack = pluginTracks;
    for (int i = 0; i <= indexTrackStack; ++i, ++pTrack) // for each plugin that can redraw
    {
      if (i > indexTrackEnable) break; // at top of active layers now

      if (!(pluginLayers[pTrack->layer].pPlugin->gettype() & PLUGIN_TYPE_REDRAW))
        continue;

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

        if (pTrack->draw.orPixelValues)
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
