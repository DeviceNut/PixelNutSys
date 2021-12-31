// Client App Command Handling
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 1 // 1 enables debugging this file

#include "main.h"

#if DEV_PATTERNS
extern void GetPrevPattern(void);
extern void GetNextPattern(void);
#endif

extern PluginFactory *pPluginFactory; // used to enumerate effect plugins

// Note: this modifies 'pattern'
void ExecPattern(char* pattern)
{
  PixelNutEngine::Status status = pPixelNutEngine->execCmdStr(pattern);
  if (status != PixelNutEngine::Status_Success)
  {
    DBGOUT((F("CmdErr: %d"), status));
    ErrorHandler(2, status, false); // blink for error and continue
  }
}

#if CLIENT_APP

#include "flash.h"

static char* skipSpaces(char* instr)
{
  while (*instr == ' ') ++instr;   // skip spaces
  return instr;
}

static char* skipNumber(char* instr)
{
  while (isdigit(*instr)) ++instr; // skip digits
  return instr;
}

static char* jsonStr(char* outstr, const char* name, const char* value, bool doterm=false)
{
  sprintf(outstr, "\"%s\":\"%s\"%s", name, value, doterm ? "}" : ",");
  return outstr;
}

static char* jsonNum(char* outstr, const char* name, int value, bool doterm=false)
{
  sprintf(outstr, "\"%s\":%d%s", name, value, doterm ? "}" : ",");
  return outstr;
}

static char* jsonArrayStart(char* outstr, const char* name)
{
  sprintf(outstr, "\"%s\":[{", name);
  return outstr;
}

static char* jsonArraydEnd(char* outstr)
{
  sprintf(outstr, "],");
  return outstr;
}

static int calcPlugins(void)
{
  int pcount = 0;
  byte *plist = pPluginFactory->pluginList();

  if (plist != NULL)
    for (int i = 0; plist[i] != 0; ++i)
      ++pcount;

  return pcount;
}

#if (STRAND_COUNT > 1)
static void loadStrandPattern(void)
{
  int curstrand = FlashSetStrand(0);
  for (int i = 0; i < STRAND_COUNT; ++i)
  {
    if (i != curstrand)
    {
      FlashSetStrand(i);
      LoadCurPattern();
    }
  }
  FlashSetStrand(curstrand);
}
#endif

void ExecAppCmd(char* instr)
{
  DBGOUT((F("CmdExec: \"%s\""), instr));

  instr = skipSpaces(instr); // skip leading spaces

  if (*instr) switch (instr[0])
  {
    default:
    {
      if (isalpha(instr[0])) ExecPattern(instr);

      else { DBGOUT((F("Unknown command: %s"), instr)); }
      break;
    }
    case '?': // sends reply in JSON format
    {
      char outstr[FLASHLEN_PATSTR+1];
      char patstr[FLASHLEN_PATSTR+1];
      char patname[FLASHLEN_PATNAME+1];

      int plugins = calcPlugins();

      pCustomCode->sendReply((char*)"?<");
      pCustomCode->sendReply((char*)"{");

      pCustomCode->sendReply( jsonNum(outstr, "nstrands",   STRAND_COUNT) );
      pCustomCode->sendReply( jsonNum(outstr, "maxstrlen",  MAXLEN_PATSTR) );
      pCustomCode->sendReply( jsonNum(outstr, "numlayers",  NUM_PLUGIN_LAYERS) );
      pCustomCode->sendReply( jsonNum(outstr, "numtracks",  NUM_PLUGIN_TRACKS) );
      pCustomCode->sendReply( jsonNum(outstr, "nplugins",   plugins) );

      #if DEV_PATTERNS
      pCustomCode->sendReply( jsonNum(outstr, "npatterns",  codePatterns) );
      #else
      pCustomCode->sendReply( jsonNum(outstr, "npatterns",  0) );
      #endif
      pCustomCode->sendReply( jsonArrayStart(outstr, "strands") );

      byte pixcounts[] = PIXEL_COUNTS;
      int curstrand = FlashSetStrand(0);

      for (int i = 0; i < STRAND_COUNT; ++i)
      {
        FlashSetStrand(i);

        pCustomCode->sendReply( jsonNum(outstr, "pixels",   pixcounts[i]) );
        pCustomCode->sendReply( jsonNum(outstr, "bright",   FlashGetValue(FLASHOFF_SDATA_PC_BRIGHT)) );
        pCustomCode->sendReply( jsonNum(outstr, "delay",    FlashGetValue(FLASHOFF_SDATA_PC_DELAY)) );
        pCustomCode->sendReply( jsonNum(outstr, "first",    FlashGetValue(FLASHOFF_SDATA_FIRSTPOS)) );
        pCustomCode->sendReply( jsonNum(outstr, "xt_mode",  FlashGetValue(FLASHOFF_SDATA_XT_MODE)) );
        pCustomCode->sendReply( jsonNum(outstr, "xt_hue",   FlashGetValue(FLASHOFF_SDATA_XT_HUE)) );
        pCustomCode->sendReply( jsonNum(outstr, "xt_white", FlashGetValue(FLASHOFF_SDATA_XT_WHT)) );
        pCustomCode->sendReply( jsonNum(outstr, "xt_count", FlashGetValue(FLASHOFF_SDATA_XT_CNT)) );
        pCustomCode->sendReply( jsonNum(outstr, "force",    FlashGetValue(FLASHOFF_SDATA_FORCE)) );

        FlashGetPatName(patname);
        pCustomCode->sendReply( jsonStr(outstr, "patname", patname) );

        // returns empty string on error
        if (!pPixelNutEngine->makeCmdStr(patstr, MAXLEN_PATSTR))
          ErrorHandler(4, 1, false); // blink for error and continue

        jsonStr(outstr, "patstr", patstr, true);
        if (i+1 < STRAND_COUNT) strcat(outstr, ",{");
        pCustomCode->sendReply(outstr);
      }
      FlashSetStrand(curstrand); // restore current strand

      pCustomCode->sendReply( jsonArraydEnd(outstr) );

      #if DEV_PATTERNS
      pCustomCode->sendReply( jsonArrayStart(outstr, "patterns") );

      for (int i = 0; i < codePatterns; ++i)
      {

        pCustomCode->sendReply( jsonStr(outstr, "name", devPatNames[i]) );
        pCustomCode->sendReply( jsonStr(outstr, "desc", devPatDesc[i]) );

        jsonStr(outstr, "pcmd", devPatCmds[i], true);
        if (i+1 < codePatterns) strcat(outstr, ",{");
        pCustomCode->sendReply(outstr);
      }

      pCustomCode->sendReply( jsonArraydEnd(outstr) );
      #endif

      #if DEV_PLUGINS
      pCustomCode->sendReply( jsonArrayStart(outstr, "plugins") );

      byte *plist = pPluginFactory->pluginList();
      for (int i = 0; plist[i] != 0; ++i)
      {
        uint16_t plugin = plist[i];
        pCustomCode->sendReply( jsonStr(outstr, "name", pPluginFactory->pluginName(plugin)) );
        pCustomCode->sendReply( jsonStr(outstr, "desc", pPluginFactory->pluginDesc(plugin)) );

        sprintf(patname, "%04X", pPluginFactory->pluginBits(plugin));
        pCustomCode->sendReply( jsonStr(outstr, "bits", patname) );

        jsonNum(outstr, "id", plugin, true);
        if (plist[i+1] != 0) strcat(outstr, ",{");
        pCustomCode->sendReply(outstr);
      }

      pCustomCode->sendReply( jsonArraydEnd(outstr) );
      #endif

      pCustomCode->sendReply( jsonNum(outstr, "version", PIXELNUT_VERSION, true) );

      pCustomCode->sendReply((char*)">?");
      break;
    }
    case '<': // return current pattern to client
    {
      char outstr[MAXLEN_PATSTR];

      if (pPixelNutEngine->makeCmdStr(outstr, MAXLEN_PATSTR))
      {
        ErrorHandler(4, 1, false); // blink for error and continue
        break;
      }

      pCustomCode->sendReply(outstr);
      break;
    }
    case '>': // store current pattern to flash
    {
      char outstr[MAXLEN_PATSTR];

      if (pPixelNutEngine->makeCmdStr(outstr, MAXLEN_PATSTR))
      {
        ErrorHandler(4, 1, false); // blink for error and continue
        break;
      }

      FlashSetPatStr(outstr);
      break;
    }
    case '*': // clear pattern
    {
      pPixelNutEngine->clearStacks(); // clear stack to prepare for new pattern
      break;
    }
    case '$': // restart: clear, then execute pattern stored in flash
    {
      char cmdstr[MAXLEN_PATSTR+1];
      pPixelNutEngine->clearStacks(); // clear stack to prepare for new pattern
      FlashGetPatStr(cmdstr); // get pattern string previously stored in flash
      ExecPattern(cmdstr);
      break;
    }
    case '=': // store pattern string to flash
    {
      FlashSetPatStr(instr+1);
      break;
    }
    case '~': // store pattern name to flash
    {
      FlashSetPatName(instr+1);
      break;
    }
    case '#': // client is switching strands
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
    case '+': // set next custom pattern
    {
      #if DEV_PATTERNS
      GetNextPattern();
      #if (STRAND_COUNT > 1)
      loadStrandPattern();
      #endif
      #endif
      break;
    }
    case '-': // set prev custom pattern
    {
      #if DEV_PATTERNS
      GetPrevPattern();
      #if (STRAND_COUNT > 1)
      loadStrandPattern();
      #endif
      #endif
      break;
    }
    case '%': // set brightness percentage
    {
      pPixelNutEngine->setBrightPercent(atoi(instr+1));
      FlashSetBright();
      break;
    }
    case '&': // set delay percentage
    {
      pPixelNutEngine->setDelayPercent( atoi(instr+1) );
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
    case '|': // set external mode on/off
    {
      bool mode = (atoi(instr+1) != 0);
      FlashSetXmode(mode);
      pPixelNutEngine->setPropertyMode(mode);
      break;
    }
    case '{': // set color hue/white and count properties
    {
      ++instr; // skip past '+'
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
      uint32_t force = atoi(instr+1);
      if (force <= MAX_FORCE_VALUE)
      {
        pPixelNutEngine->triggerForce(force);
        FlashSetForce(force);
      }
      else ErrorHandler(4, 1, false); // blink for error and continue
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
