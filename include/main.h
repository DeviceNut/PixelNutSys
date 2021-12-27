/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#pragma once

#include "core.h"
#include "custom.h"

extern byte codePatterns;
extern bool doUpdate;
#if !CLIENT_APP
extern byte curPattern;
#endif

extern CustomCode *pCustomCode;
extern PixelNutEngine pixelNutEngines[STRAND_COUNT];
extern PixelNutEngine *pPixelNutEngine;

extern void BlinkStatusLED(uint16_t slow, uint16_t fast);
extern void ErrorHandler(short slow, short fast, bool dostop);

extern void ExecAppCmd(char *cmdstr);
extern void ExecPattern(char *pattern);

#if DEV_PATTERNS
extern const char* const devPatCmds[];
extern void CountPatterns(void);
#if CLIENT_APP
extern const char* const devPatNames[];
extern const char* const devPatDesc[];
#endif
#elif !CLIENT_APP
#error("Must have device patterns if no client");
#endif
