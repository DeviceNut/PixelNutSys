/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#pragma once

#include "core.h"

class CustomCode
{
public:
  // called during setup()
  virtual void setup(void) {}
  
  // called during loop()
  virtual void loop(void) {}

  // set name for device
  virtual void setName(char *name) {}

  // send reply to client
  virtual void sendReply(char *instr) {}
};
extern CustomCode *pCustomCode;

extern PixelNutEngine pixelNutEngines[STRAND_COUNT];
extern PixelNutEngine *pPixelNutEngine;

extern void BlinkStatusLED(uint16_t slow, uint16_t fast);
extern void ErrorHandler(short slow, short fast, bool dostop);

extern void ExecAppCmd(char *cmdstr);
extern void ExecPattern(char *pattern);

extern bool doUpdate;

#if DEV_PATTERNS
extern byte codePatterns;
extern byte curPattern;
extern const char* const devPatCmds[];
extern void CountPatterns(void);
#if CLIENT_APP
extern const char* const devPatNames[];
extern const char* const devPatDesc[];
#endif
#elif !CLIENT_APP
#error("Must have device patterns if no client");
#endif
