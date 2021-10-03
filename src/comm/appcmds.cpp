// Client App Command Handling
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"

void ExecPattern(char *pattern)
{
  PixelNutEngine::Status status = pPixelNutEngine->execCmdStr(pattern);
  if (status != PixelNutEngine::Status_Success)
  {
    DBGOUT((F("CmdErr: %d"), status));
    ErrorHandler(2, status, false); // blink for error and continue
  }

  *pattern = 0; // must clear command string after finished
}

#if CLIENT_APP

#include "flash.h"

static char* skipSpaces(char *instr)
{
  while (*instr == ' ') ++instr;   // skip spaces
  return instr;
}

static char* skipNumber(char *instr)
{
  while (isdigit(*instr)) ++instr; // skip digits
  return instr;
}

void AddNumToStr(char *outstr, int value)
{
  char numstr[10];
  itoa(value, numstr, 10);
  strcat(outstr, numstr);
  strcat(outstr, " ");
}

void ExecAppCmd(char *instr)
{
  DBGOUT((F("CmdExec: \"%s\""), instr));

  static int16_t saveStrIndex = -1;

  if (saveStrIndex < 0) instr = skipSpaces(instr); // skip leading spaces
  // else need those separating spaces while in sequence

  if (*instr) switch (instr[0])
  {
    default:
    {
      if (saveStrIndex >= 0) // save part of pattern into flash
      {
        FlashSetStr(instr, saveStrIndex);
        saveStrIndex += strlen(instr);
      }
      else if (isalpha(instr[0])) ExecPattern(instr);

      else { DBGOUT((F("Unknown command: %s"), instr)); }
      break;
    }
    case '.': // start/stop saving a pattern into flash
    {
      char pattern[STRLEN_PATTERNS];

      if ( saveStrIndex >= 0)  // end sequence
      {
        saveStrIndex = -1;
        pPixelNutEngine->clearStack(); // clear stack to prepare for new pattern
        FlashGetStr(pattern);
        ExecPattern(pattern);
      }
      else saveStrIndex = 0; // begin sequence
      break;
    }
    case '?': // sends reply with configuration strings
    {
      char outstr[STRLEN_PATTERNS];

      #if (STRAND_COUNT > 1)
      if (instr[1] == 'S') // send info about segments
      {
        for (int i = 0; i < STRAND_COUNT; ++i)
        {
          FlashSetStrand(i);

          outstr[0] = 0;
          for (int j = 0; j < FLASH_SEG_LENGTH; ++j)
            AddNumToStr(outstr, FlashGetValue(j));

          DBGOUT((F("FlashVals[%d]: %s"), i, outstr));

          pCustomCode->sendReply(outstr);
        }

        FlashSetStrand(0); // MUST start with strand 0
        break;
      }
      #endif

      #if DEV_PATTERNS
      if (instr[1] == 'P') // about internal patterns
      {
        DBGOUT((F("Patterns:  %d"), codePatterns));

        for (int i = 0; i < codePatterns; ++i)
        {
          pCustomCode->sendReply((char*)devPatNames[i]);
          pCustomCode->sendReply((char*)devPatDesc[i]);
          pCustomCode->sendReply((char*)devPatCmds[i]);
        }
        break;
      }
      #endif

      #if DEV_PLUGINS
      if (instr[1] == 'G') // about internal plugins
      {
        // TODO: send info on each built-in plugin
        break;
      }
      #endif

      if (instr[1] != 0)
      {
        DBGOUT((F("Unknown ? modifier: %c"), instr[1]));
        pCustomCode->sendReply((char*)"?");
        break; // don't return false for error
      }

      char reply[1000]; // FIXME?
      char *rstr = reply;
      byte pixcounts[] = PIXEL_COUNTS;

      sprintf(rstr, "P!!\n%d %d %d %d %d %d\n", 
                      STRAND_COUNT, STRLEN_PATTERNS, codePatterns, 0, // custom plugin count TODO
                      NUM_PLUGIN_LAYERS, NUM_PLUGIN_TRACKS);
      rstr += strlen(rstr);

      int curstrand = FlashSetStrand(0);

      for (int i = 0; i < STRAND_COUNT; ++i)
      {
        FlashSetStrand(i);
        FlashGetStr(outstr);

        uint16_t hue = FlashGetValue(FLASH_SEG_XT_HUE) + (FlashGetValue(FLASH_SEG_XT_HUE+1) << 8);

        sprintf(rstr, "%d %d %d %d\n%d %d %d %d %d\n%s",
                      pixcounts[i],
                      FlashGetValue(FLASH_SEG_BRIGHTNESS),
                      (int8_t)FlashGetValue(FLASH_SEG_DELAYMSECS),
                      FlashGetValue(FLASH_SEG_FIRSTPOS),

                      FlashGetValue(FLASH_SEG_XT_MODE),
                      hue,
                      FlashGetValue(FLASH_SEG_XT_WHT),
                      FlashGetValue(FLASH_SEG_XT_CNT),
                      FlashGetValue(FLASH_SEG_FORCE),
                      outstr);
        rstr += strlen(rstr);
      }
      pCustomCode->sendReply(reply);

      FlashSetStrand(curstrand); // restore current strand
      break;
    }
    case '#': // client is switching physical segments
    {
      #if (STRAND_COUNT > 1)
      byte index = *(instr+1)-0x30; // convert ASCII digit to value
      if (index < STRAND_COUNT)
      {
        DBGOUT((F("Switching to strand #%d"), index));
        FlashSetStrand(index);
        pPixelNutEngine = pixelNutEngines[index];
      }
      #endif
      break;
    }
    case '%': // set maximum brightness level
    {
      pPixelNutEngine->setMaxBrightness(atoi(instr+1));
      FlashSetBright();
      break;
    }
    case ':': // set delay offset
    {
      pPixelNutEngine->setDelayOffset( atoi(instr+1) );
      FlashSetDelay();
      break;
    }
    case '^': // set first position
    {
      pPixelNutEngine->setFirstPosition( atoi(instr+1) );
      FlashSetFirst();
      break;
    }
    case '[': // pause display updating
    {
      doUpdate = false;
      break;
    }
    case ']': // resume display updating
    {
      doUpdate = true;
      break;
    }
    case '_': // set external mode on/off
    {
      bool mode = (atoi(instr+1) != 0);
      FlashSetXmode(mode);
      pPixelNutEngine->setPropertyMode(mode);
      break;
    }
    case '=': // set color hue/white and count properties
    {
      ++instr; // skip past '='
      short hue = atoi(instr);

      instr = skipNumber(instr); // skip number digits
      instr = skipSpaces(instr); // skip leading spaces
      byte wht = atoi(instr);

      instr = skipNumber(instr); // skip number digits
      instr = skipSpaces(instr); // skip leading spaces
      byte cnt = atoi(instr);

      FlashSetExterns(hue, wht, cnt);

      pPixelNutEngine->setColorProperty(hue, wht);
      pPixelNutEngine->setCountProperty(cnt);
      break;
    }
    case '!': // causes a trigger given specified force
    {
      short force = atoi(instr+1);
      pPixelNutEngine->triggerForce(force);
      FlashSetForce(force);
      break;
    }
    case '@': // change the device name
    {
      ++instr; // skip '@'
      instr[MAXLEN_DEVICE_NAME] = 0; // limit length

      DBGOUT((F("Seting device name: \"%s\""), instr));
      pCustomCode->setName(instr);
      break;
    }
  }
}

#endif // CLIENT_APP
