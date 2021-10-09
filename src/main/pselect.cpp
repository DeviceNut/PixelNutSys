/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"
#include "flash.h"

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
    strcpy_P(curPatStr, devPatCmds[i]);
    DBGOUT((F("  %2d: \"%s\""), i+1, curPatStr));
    #else
    DBGOUT((F("  %2d: %s"), i+1, devPatNames[i]));
    #endif
  }

  // cannot continue if cannot find any patterns
  if (!codePatterns) ErrorHandler(1, 1, true);
}
#endif

#if !CLIENT_APP // else controlled from client app

void LoadCurPattern()
{
  if ((0 < curPattern) && (curPattern <= codePatterns))
  {
    strcpy_P(curPatStr, devPatCmds[curPattern-1]);
    DBGOUT((F("Retrieving device pattern #%d"), curPattern));

    pPixelNutEngine->clearStack(); // clear stack to prepare for new pattern
    ExecPattern(curPatStr);
  }
}

void GetNextPattern(void)
{
  // curPattern must be 1...codePatterns
  if (++curPattern > codePatterns)
    curPattern = 1;

  LoadCurPattern();
}

void GetPrevPattern(void)
{
  // curPattern must be 1...codePatterns
  if (curPattern <= 1) curPattern = codePatterns;
  else --curPattern;

  LoadCurPattern();
}

#endif // !CLIENT_APP

