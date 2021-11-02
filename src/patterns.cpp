/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"

#if DEV_PATTERNS

#if defined(ESP32)
#define PROGMEM 
#endif

#if PLUGIN_PLASMA
#define PLASMA_NAME "Plasma"
#define PLASMA_DESC "Creates a \"plasma\" type of effect by using Lissajious curve mathematics. Increase the Count Property for faster motion."
static PROGMEM const char PLASMA_PATTERN[] = "E80 Q4 T E120 F100 I T G";
#endif

#if PLUGIN_SPECTRA
#define SPECTRUM_NAME "Spectrum"
#define SPECTRUM_DESC "Creates a \"spectrum\" effect using an FFT from audio data out of a microphone."
static PROGMEM const char SPECTRUM_PATTERN[] = "E0 B60 H250 W50 T E70 V T G";
#endif

#if CLIENT_APP

const char* const devPatNames[] =
{
  #if PLUGIN_PLASMA
  PLASMA_NAME
  #endif

  #if PLUGIN_SPECTRA
  SPECTRUM_NAME
  #endif
};

const char* const devPatDesc[] =
{
  #if PLUGIN_PLASMA
  PLASMA_DESC
  #endif

  #if PLUGIN_SPECTRA
  SPECTRUM_DESC
  #endif
};

#endif // CLIENT_APP

const char* const devPatCmds[] =
{
  #if PLUGIN_PLASMA
  PLASMA_PATTERN,
  #endif

  #if PLUGIN_SPECTRA
  SPECTRUM_PATTERN,
  #endif
  
  0 // NULL terminator
};

#endif // DEV_PATTERNS