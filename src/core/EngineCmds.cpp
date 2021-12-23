// PixelNut Engine Class Implementation of Command functions
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 1 // 1 enables debugging this file (must also set in main.h)

#include "core.h"
#include "PixelNutPlugin.h"
#include "PixelNutEngine.h"

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

// clips values to range 0-'maxval' (maxval=0 for full 16 bits)
// returns 'curval' if no value is specified
static short GetNumValue(char *str, int curval, int maxval)
{
  if ((str == NULL) || !isdigit(*str)) return curval;
  int newval = atoi(str);
  if (maxval && (newval > maxval)) return maxval;
  if (newval < 0) return 0;
  return newval;
}

PixelNutEngine::Status PixelNutEngine::execCmdStr(char *cmdstr)
{
  Status status = Status_Success;

  short curlayer = indexLayerStack;

  for (int i = 0; cmdstr[i]; ++i) // convert to upper case
    cmdstr[i] = toupper(cmdstr[i]);

  char *cmd = strtok(cmdstr, " "); // separate options by spaces

  if (cmd == NULL) return Status_Success; // ignore empty line
  do
  {
    PixelNutSupport::DrawProps *pdraw = NULL;
    if (curlayer >= 0) pdraw = &pluginLayers[curlayer].pTrack->draw;

    DBGOUT((F(">> Cmd=%s len=%d layer=%d"), cmd, strlen(cmd), curlayer));

    if (cmd[0] == 'P') // Pop all plugins from the stack
    {
      clearStacks();
      curlayer = indexLayerStack;
    }
    else if (cmd[0] == 'L') // set plugin layer to modify ('L' uses top of stack)
    {
      int layer = GetNumValue(cmd+1, indexLayerStack); // returns -1 if not in range

      if (layer >= 0)
      {
        DBGOUT((F("LayerCmd: cur=%d max=%d"), layer, indexLayerStack));
        curlayer = layer;        
      }
      else
      {
        DBGOUT((F("Layer %d not valid: max=%d"), layer, indexLayerStack));
        status = Status_Error_BadVal;
      }
    }
    else if (cmd[0] == 'E') // add a plugin Effect to the stack ("E" is an error)
    {
      int plugin = GetNumValue(cmd+1, MAX_PLUGIN_VALUE); // returns -1 if not in range
      status = AppendPluginLayer((uint16_t)plugin);
      curlayer = indexLayerStack;
    }
    else if (pdraw != NULL)
    {
      switch (cmd[0])
      {
        case 'M': // sets/clears mute state for track/layer ("M" same as "M1")
        {
          bool disable = GetBoolValue(cmd+1, true);
          DBGOUT((F("Layer=%d Disable=%d"), curlayer, disable));
          PluginLayer *pLayer = (pluginLayers + curlayer);
          pLayer->disable = disable;
          if (disable && pLayer->redraw)
            memset(TRACK_BUFFER(pLayer->pTrack), 0, pixelBytes);
          break;
        }
        case 'S': // switch/swap effect for existing track ("S" swaps with next track/layer)
        {
          if (isdigit(*(cmd+1))) // there is a value after "S"
          {
            int plugin = GetNumValue(cmd+1, MAX_PLUGIN_VALUE); // returns -1 if not in range
            status = SwitchPluginLayer(curlayer, (uint16_t)plugin);
          }
          else status = SwapPluginLayers(curlayer);
          break;
        }
        case 'Z': // append/remove track/layer ("Z" to delete, else value is effect to insert)
        {
          if (isdigit(*(cmd+1))) // there is a value after "Z"
          {
            int plugin = GetNumValue(cmd+1, MAX_PLUGIN_VALUE); // returns -1 if not in range
            DBGOUT((F("Layer=%d Append plugin=%d"), curlayer, plugin));
            status = AddPluginLayer(curlayer, (uint16_t)plugin);
          }
          else DeletePluginLayer(curlayer);
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
          pdraw->pixLen = (uint16_t)GetNumValue(cmd+1, 1, numPixels);
          DBGOUT((F(">> Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'J': // offset into output display of the track by percent
        {
          uint16_t pcent = (uint16_t)GetNumValue(cmd+1, 0, MAX_PERCENTAGE);
          pdraw->pixStart = pixelNutSupport.mapValue(pcent, 0, MAX_PERCENTAGE, 0, numPixels-1);
          DBGOUT((F("Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'K': // number of pixels in the track by percent
        {
          uint16_t pcent = (uint16_t)GetNumValue(cmd+1, 0, MAX_PERCENTAGE);
          pdraw->pixLen = pixelNutSupport.mapValue(pcent, 0, MAX_PERCENTAGE, 1, numPixels);
          DBGOUT((F("Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
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
          pdraw->pcentDelay = (byte)GetNumValue(cmd+1, DEF_PCENTDELAY, MAX_PERCENTAGE);
          DBGOUT((F("Delay=%d%%"), pdraw->pcentDelay));
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
            pluginLayers[curlayer].pTrack->ctrlBits = bits;
            if (externPropMode)
            {
              if (bits & ExtControlBit_DegreeHue)
              {
                pdraw->degreeHue = externDegreeHue;
                DBGOUT((F("SetExtern: layer=%d hue=%d"), curlayer, externDegreeHue));
              }

              if (bits & ExtControlBit_PcentWhite)
              {
                pdraw->pcentWhite = externPcentWhite;
                DBGOUT((F("SetExtern: layer=%d white=%d"), curlayer, externPcentWhite));
              }

              if (bits & ExtControlBit_PixCount)
              {
                pdraw->pixCount = pixelNutSupport.mapValue(externPcentCount, 0,
                                                MAX_PERCENTAGE, 1, numPixels);
                DBGOUT((F("SetExtern: layer=%d count=%d"), curlayer, pdraw->pixCount));
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
        case 'F': // force value to be used by trigger ("F" for random force)
        {
          if (isdigit(*(cmd+1))) // there is a value after "F" (clip to 0-MAX_FORCE_VALUE)
               pluginLayers[curlayer].trigForce = GetNumValue(cmd+1, 0, MAX_FORCE_VALUE);
          else pluginLayers[curlayer].trigForce = -1; // get random value each time
          break;
        }
        case 'T': // trigger plugin layer ("T0" to disable, "T" same as "T1")
        {
          if (GetBoolValue(cmd+1, true))
          {
            pluginLayers[curlayer].trigType |= TrigTypeBit_AtStart;
            short force = pluginLayers[curlayer].trigForce;
            if (force < 0) force = random(0, MAX_FORCE_VALUE+1);
            TriggerLayer((pluginLayers + curlayer), force); // trigger immediately
          }
          else pluginLayers[curlayer].trigType &= ~TrigTypeBit_AtStart;
          break;
        }
        case 'I': // external triggering enable ("I0" to disable, "I" same as "I1")
        {
          if (GetBoolValue(cmd+1, true))
               pluginLayers[curlayer].trigType |=  TrigTypeBit_External;
          else pluginLayers[curlayer].trigType &= ~TrigTypeBit_External;
          break;
        }
        case 'A': // assign effect layer as trigger source ("A" disables)
        {
          if (isdigit(*(cmd+1))) // there is a value after "A"
          {
            pluginLayers[curlayer].trigType |= TrigTypeBit_Internal;
            pluginLayers[curlayer].trigLayerIndex = (byte)GetNumValue(cmd+1, MAX_BYTE_VALUE, MAX_BYTE_VALUE);
            DBGOUT((F("Triggering assigned to layer %d"), pluginLayers[curlayer].trigLayerIndex));
            // MUST assign tigLayerID for this layer when activated
          }
          else pluginLayers[curlayer].trigType &= ~TrigTypeBit_Internal;
          break;
        }
        case 'R': // sets repeat trigger ("R0" to disable, "R" for forever count, else "R<count>")
        {
          bool enable = true;

          if (isdigit(*(cmd+1))) // there is a value after "R"
          {
            uint16_t value = (uint16_t)GetNumValue(cmd+1, 0, 0);
            if (value == 0) enable = false;
            else pluginLayers[curlayer].trigRepCount = value;
          }

          if (enable)
          {
            pluginLayers[curlayer].trigType |= TrigTypeBit_Repeating;
            pluginLayers[curlayer].trigDnCounter = pluginLayers[curlayer].trigRepCount;

            pluginLayers[curlayer].trigTimeMsecs = pixelNutSupport.getMsecs() +
                (1000 * random(pluginLayers[curlayer].trigRepOffset,
                              (pluginLayers[curlayer].trigRepOffset +
                               pluginLayers[curlayer].trigRepRange+1)));

            DBGOUT((F("RepeatTrigger: layer=%d offset=%u range=%d count=%d force=%d"), curlayer,
                      pluginLayers[curlayer].trigRepOffset, pluginLayers[curlayer].trigRepRange,
                      pluginLayers[curlayer].trigRepCount, pluginLayers[curlayer].trigForce));
          }
          else pluginLayers[curlayer].trigType &= ~TrigTypeBit_Repeating;
          break;
        }
        case 'O': // repeat trigger offset time ("O" sets default value)
        {
          pluginLayers[curlayer].trigRepOffset = (uint16_t)GetNumValue(cmd+1, DEF_TRIG_OFFSET, 0);
          break;
        }
        case 'N': // range of trigger time ("N" sets default value)
        {
          pluginLayers[curlayer].trigRepRange = (uint16_t)GetNumValue(cmd+1, DEF_TRIG_RANGE, 0);
          break;
        }
        case 'G': // Go: activate all effects in entire pattern
        {
          if (!patternEnabled)
          {
            DBGOUT((F("Activate %d tracks"), indexTrackStack+1));
            patternEnabled = true;

            // assign trigLayerID for layers with TrigTypeBit_Internal set
            PluginLayer *pLayer = pluginLayers;
            for (int i = 0; i <= indexLayerStack; ++i, ++pLayer)
            {
              if (pLayer->trigType & TrigTypeBit_Internal)
              {
                int index = pLayer->trigLayerIndex;
                if (index > indexLayerStack)
                {
                  DBGOUT((F("Invalid trigger index=%d for layer=%d"), index, i));
                  pLayer->trigType &= ~TrigTypeBit_Internal;
                }
                else pLayer->trigLayerID = pluginLayers[index].thisLayerID;
              }
            }
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
    PluginTrack *pTrack = pLayer->pTrack;
    PixelNutSupport::DrawProps *pdraw = &pTrack->draw;

    sprintf(str, "E%d ", pLayer->iplugin);
    if (!addstr(&cmdstr, str, &addlen)) goto error;

    if (pLayer->disable)
    {
      sprintf(str, "M ");
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }

    if (pLayer->redraw) // drawing layer
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

      if (pdraw->pcentDelay != DEF_PCENTDELAY)
      {
        sprintf(str, "D%d ", pdraw->pcentDelay);
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
      sprintf(str, "A%d ", pLayer->trigLayerIndex);
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

    DBGOUT((F("Make: layer=%d plugin=%d str=\"%s\""), i, pLayer->iplugin, basestr));
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
