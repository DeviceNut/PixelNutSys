// PixelNut Engine Class Implementation of Command functions
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 1 // 1 enables debugging this file

#include "core.h"

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
  bool neweffects = false;

  for (int i = 0; cmdstr[i]; ++i) // convert to upper case
    cmdstr[i] = toupper(cmdstr[i]);

  char *cmd = strtok(cmdstr, " "); // separate options by spaces

  if (cmd == NULL) return Status_Success; // ignore empty line
  do
  {
    PixelNutSupport::DrawProps *pdraw = NULL;
    if (curlayer >= 0) pdraw = &pluginLayers[curlayer].pTrack->draw;

    DBGOUT((F("ExecCmd: \"%s\" Len=%d Layer=%d"), cmd, strlen(cmd), curlayer));

    if (cmd[0] == 'L') // set plugin layer to modify ('L' uses top of stack)
    {
      int layer = GetNumValue(cmd+1, indexLayerStack); // returns -1 if not in range

      if (layer >= 0)
      {
        DBGOUT((F("  Layer Cur=%d Max=%d"), layer, indexLayerStack));
        curlayer = layer;        
      }
      else
      {
        DBGOUT((F("  Layer %d not valid: Max=%d"), atoi(cmd+1), indexLayerStack));
        status = Status_Error_BadVal;
      }
    }
    else if (cmd[0] == 'E') // add a plugin Effect to the stack ("E" is an error)
    {
      int plugin = GetNumValue(cmd+1, MAX_PLUGIN_VALUE); // returns -1 if not in range
      status = AppendPluginLayer((uint16_t)plugin);
      curlayer = indexLayerStack;
      neweffects = true;
    }
    else if (pdraw != NULL)
    {
      switch (cmd[0])
      {
        case 'M': // sets/clears mute/solor states for track/layer ("M" same as "M1")
        {
          short value = GetNumValue(cmd+1, (ENABLEBIT_MUTE | ENABLEBIT_SOLO));
          if (value < 0) value = ENABLEBIT_MUTE;
          DBGOUT((F("  Layer=%d Mute/Solo=%d"), curlayer, value));

          PluginLayer *pLayer = (pluginLayers + curlayer);
          pLayer->solo = !!(value & ENABLEBIT_SOLO);
          pLayer->mute = !!(value & ENABLEBIT_MUTE);

          if (pLayer->mute && pLayer->redraw)
            memset(TRACK_BUFFER(pLayer->pTrack), 0, pixelBytes);

          neweffects = true;
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

          neweffects = true;
          break;
        }
        case 'Z': // append/remove track/layer ("Z" to delete, else value is effect to insert)
        {
          if (isdigit(*(cmd+1))) // there is a value after "Z"
          {
            int plugin = GetNumValue(cmd+1, MAX_PLUGIN_VALUE); // returns -1 if not in range
            DBGOUT((F("  Layer=%d Append plugin=%d"), curlayer, plugin));
            status = AddPluginLayer(curlayer, (uint16_t)plugin);
          }
          else
          {
            DeletePluginLayer(curlayer);
            curlayer = indexLayerStack;
          }

          neweffects = true;
          break;
        }
        case 'X': // offset into output display of the track by pixel index
        {
          pdraw->pixStart = (uint16_t)GetNumValue(cmd+1, 0, numPixels-1);
          DBGOUT((F("  Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'Y': // number of pixels in the track by pixel index
        {
          pdraw->pixLen = (uint16_t)GetNumValue(cmd+1, 1, numPixels);
          DBGOUT((F("  Start=%d Len=%d"), pdraw->pixStart, pdraw->pixLen));
          break;
        }
        case 'J': // offset into output display of the track by percent
        {
          uint16_t pcent = (uint16_t)GetNumValue(cmd+1, 0, MAX_PERCENTAGE);
          pdraw->pcentStart = pcent;
          pdraw->pixStart = pixelNutSupport.mapValue(pcent, 0, MAX_PERCENTAGE, 0, numPixels-1);
          DBGOUT((F("  PixStart: %d%% => %d"), pcent, pdraw->pixStart));
          break;
        }
        case 'K': // number of pixels in the track by percent (0 for rest of the strand)
        {
          uint16_t pcent = (uint16_t)GetNumValue(cmd+1, 0, MAX_PERCENTAGE);
          pdraw->pcentLen = pcent;
          if (pcent == 0) pdraw->pixLen = numPixels - pdraw->pixStart;
          else pdraw->pixLen = pixelNutSupport.mapValue(pcent, 0, MAX_PERCENTAGE, 1, numPixels);
          DBGOUT((F("  PixLen: %d%% => %d"), pcent, pdraw->pixLen));
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
          DBGOUT((F("  Delay=%d%%"), pdraw->pcentDelay));
          break;
        }
        case 'H': // color Hue value property ("H" sets default value)
        {
          pdraw->dvalueHue = (uint16_t)GetNumValue(cmd+1, DEF_DVALUE_HUE, MAX_DVALUE_HUE);
          pixelNutSupport.makeColorVals(pdraw);
          break;
        }
        case 'W': // percent White property ("W" sets default value)
        {
          pdraw->pcentWhite = (byte)GetNumValue(cmd+1, DEF_PCENTWHITE, MAX_PERCENTAGE);
          pixelNutSupport.makeColorVals(pdraw);
          break;
        }
        case 'C': // percent PixelCount property ("C" sets default value)
        {
          uint16_t pcent = (uint16_t)GetNumValue(cmd+1, DEF_PCENTCOUNT, MAX_PERCENTAGE);
          pdraw->pcentCount = pcent;
          pdraw->pixCount = pixelNutSupport.mapValue(pcent, 0, MAX_PERCENTAGE, 1, numPixels);
          DBGOUT((F("  PixCount: %d%% => %d"), pcent, pdraw->pixCount));
          break;
        }
        case 'Q': // extern control bits ("Q" is same as "Q0")
        {
          short bits = GetNumValue(cmd+1, ExtControlBit_All); // returns -1 if not within range
          pluginLayers[curlayer].pTrack->ctrlBits = bits;
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
        case 'G': // do not repeat ("G" for not default, else sets value)
        {
          pdraw->noRepeating = GetBoolValue(cmd+1, !DEF_NOREPEATING);
          // must now restart the track to be effective
          status = SwitchPluginLayer(curlayer, pluginLayers[curlayer].iplugin);
          break;
        }
        case 'F': // force value to be used by trigger ("F" for random force)
        {
          if (isdigit(*(cmd+1))) // there is a value after "F" (clip to 0-MAX_FORCE_VALUE)
          {
            pluginLayers[curlayer].trigForce = GetNumValue(cmd+1, 0, MAX_FORCE_VALUE);
            pluginLayers[curlayer].randForce = false;
          }
          else pluginLayers[curlayer].randForce = true; // get random value each time
          break;
        }
        case 'T': // trigger plugin layer ("T0" to disable, "T" same as "T1")
        {
          if (GetBoolValue(cmd+1, true))
          {
            byte force;
            pluginLayers[curlayer].trigType |= TrigTypeBit_AtStart;
            if (pluginLayers[curlayer].randForce)
                 force = random(0, MAX_FORCE_VALUE+1);
            else force = pluginLayers[curlayer].trigForce;
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
            pluginLayers[curlayer].trigLayerIndex =
                (byte)GetNumValue(cmd+1, MAX_LAYER_VALUE, MAX_LAYER_VALUE);

            DBGOUT((F("  Triggering for layer=%d assigned to layer=%d"),
                    curlayer, pluginLayers[curlayer].trigLayerIndex));

            pluginLayers[curlayer].trigType |= TrigTypeBit_Internal;
          }
          else pluginLayers[curlayer].trigType &= ~TrigTypeBit_Internal;

          neweffects = true;
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
          else pluginLayers[curlayer].trigRepCount = 0;

          if (enable)
          {
            pluginLayers[curlayer].trigType |= TrigTypeBit_Repeating;
            pluginLayers[curlayer].trigDnCounter = pluginLayers[curlayer].trigRepCount;

            pluginLayers[curlayer].trigTimeMsecs = pixelNutSupport.getMsecs() +
                (1000 * random(pluginLayers[curlayer].trigRepOffset,
                              (pluginLayers[curlayer].trigRepOffset +
                               pluginLayers[curlayer].trigRepRange+1)));

            DBGOUT((F("  RepeatTrigger: layer=%d offset=%u range=%d count=%d force=%d"), curlayer,
                      pluginLayers[curlayer].trigRepOffset,
                      pluginLayers[curlayer].trigRepRange,
                      pluginLayers[curlayer].trigRepCount,
                      pluginLayers[curlayer].randForce ? -1 : pluginLayers[curlayer].trigForce));
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
        default:
        {
          status = Status_Error_BadCmd;
          break;
        }
      }
    }
    else
    {
      DBGOUT((F("!! Must add track before setting draw parms")));
      status = Status_Error_BadCmd;
    }

    if (status != Status_Success) break;

    cmd = strtok(NULL, " ");
  }
  while (cmd != NULL);

  if ((status == Status_Success) && neweffects)
  {
    // assign trigLayerID for layers with TrigTypeBit_Internal set
    PluginLayer *pLayer = pluginLayers;
    for (int i = 0; i <= indexLayerStack; ++i, ++pLayer)
    {
      if (pLayer->trigType & TrigTypeBit_Internal)
      {
        int index = pLayer->trigLayerIndex;
        if (index > indexLayerStack)
        {
          DBGOUT((F("!! Invalid trigger index=%d for layer=%d"), index, i));
          pLayer->trigType &= ~TrigTypeBit_Internal;
        }
        else pLayer->trigLayerID = pluginLayers[index].thisLayerID;
      }
    }
  }
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
    PluginTrack *pTrack = pLayer->pTrack;
    PixelNutSupport::DrawProps *pdraw = &pTrack->draw;

    sprintf(str, "E%d ", pLayer->iplugin);
    if (!addstr(&cmdstr, str, &addlen)) goto error;

    if (pLayer->solo || pLayer->mute)
    {
      int bits = (pLayer->solo ? ENABLEBIT_SOLO:0) | (pLayer->mute ? ENABLEBIT_MUTE:0);
      sprintf(str, "M%d ", bits);
      if (!addstr(&cmdstr, str, &addlen)) goto error;
    }

    if (pLayer->redraw) // drawing layer
    {
      if (pdraw->pcentStart != 0)
      {
        sprintf(str, "J%d ", pdraw->pcentStart);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->pcentLen != MAX_PERCENTAGE)
      {
        sprintf(str, "K%d ", pdraw->pcentLen);
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

      if (pdraw->dvalueHue != DEF_DVALUE_HUE)
      {
        sprintf(str, "H%d ", pdraw->dvalueHue);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->pcentWhite != DEF_PCENTWHITE)
      {
        sprintf(str, "W%d ", pdraw->pcentWhite);
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }

      if (pdraw->pcentCount != DEF_PCENTCOUNT)
      {
        sprintf(str, "C%d ", pdraw->pcentCount);
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

      if (pdraw->noRepeating != DEF_NOREPEATING)
      {
        sprintf(str, "G ");
        if (!addstr(&cmdstr, str, &addlen)) goto error;
      }
    }

    if (pLayer->trigForce != DEF_FORCEVAL)
    {
      if (pLayer->randForce) sprintf(str, "F ");
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

  return true;

error:
  DBGOUT((F("Pattern string longer than: %d"), maxlen));
  return false;
}
