/*
Copyright (c) 2024, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"
#include "main/flash.h"

#if DEV_PATTERNS

void CountPatterns(void)
{
  DBGOUT((F("Stored Patterns:")));

  codePatterns = 0;
  for (int i = 0; ; ++i)
  {
    if (devPatCmds[i] == NULL)
    {
      codePatterns = i;
      break;
    }

    #if !CLIENT_APP
    char outstr[MAXLEN_PATSTR+1];
    strcpy_P(outstr, devPatCmds[i]);
    DBGOUT((F("  %2d: \"%s\""), i+1, outstr));
    #else
    DBGOUT((F("  %2d: %s"), i+1, devPatNames[i]));
    #endif
  }

  // cannot continue if cannot find any patterns
  if (!codePatterns) ErrorHandler(1, 1, true);
}

void LoadCurPattern()
{
  char cmdstr[MAXLEN_PATSTR+1];

  pPixelNutEngine->clearStacks(); // clear stack to prepare for new pattern

  if ((1 <= curPattern) && (curPattern <= codePatterns))
  {
    DBGOUT((F("Retrieving device pattern #%d"), curPattern));
    strcpy_P(cmdstr, devPatCmds[curPattern-1]);
  }
  else
  {
    // patterns are sent from external client and stored in flash
    // the pattern number is meaningful only to the client, or if 0
    // if using physical controls to select via calls below

    DBGOUT((F("Retrieved external pattern #%d"), curPattern));
    FlashGetPatStr(cmdstr); // get pattern string previously stored in flash
  }

  ExecPattern(cmdstr);
}

void GetNextPattern(void)
{
  // allow selecting stored external pattern as part of the cycle
  if (curPattern > codePatterns) curPattern = 0; // 0 for extern
  else ++curPattern;

  LoadCurPattern();
}

void GetPrevPattern(void)
{
  // allow selecting stored external pattern as part of the cycle
  if (!curPattern) curPattern = codePatterns;
  else --curPattern;

  LoadCurPattern();
}

#endif
