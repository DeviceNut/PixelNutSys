/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"

#if DEV_PLUGINS

// extended PixelNut-Plugins (PNP):

#include "PNP_Spectra.h"
#include "PNP_Plasma.h"

// effect capability bits (these must agree with client defines)
#define PNP_EBIT_COLOR          0x0001  // changing color changes effect
#define PNP_EBIT_COUNT          0x0002  // changing count changes effect
#define PNP_EBIT_DELAY          0x0004  // changing delay changes effect
#define PNP_EBIT_DIRECTION      0x0008  // changing direction changes effect
#define PNP_EBIT_ROTATION       0x0010  // changing rotation changes effect
#define PNP_EBIT_REPTRIGS       0x0020  // repeat triggers changes effect
#define PNP_EBIT_TRIGFORCE      0x0040  // force used in triggering
#define PNP_EBIT_SENDFORCE      0x0080  // sends force to other plugins
                                        // only for filter effects:
#define PNP_EBIT_ORIDE_HUE      0x0100  // effect overrides hue property
#define PNP_EBIT_ORIDE_WHITE    0x0200  // effect overrides white property
#define PNP_EBIT_ORIDE_COUNT    0x0400  // effect overrides count property
#define PNP_EBIT_ORIDE_DELAY    0x0800  // effect overrides delay property
#define PNP_EBIT_ORIDE_DIR      0x1000  // effect overrides direction property
#define PNP_EBIT_ORIDE_EXT      0x2000  // effect overrides start/extent properties

#define PNP_EBIT_REDRAW         0x8000  // set if redraw effect, else filter

static byte plist[] =
{
  #if PLUGIN_SPECTRA
  70,
  #endif

  #if PLUGIN_PLASMA
  80,
  #endif
  0
};

class XPluginFactory : public PluginFactory
{
public:

  byte *pluginList(void) { return plist; }

  // returns name of plugin
  char *pluginName(uint16_t plugin)
  {
    switch (plugin)
    {
      #if PLUGIN_SPECTRA
      case 70: return char*)"Spectra";
      #endif

      #if PLUGIN_PLASMA
      case 80: return (char*)"Plasma";
      #endif

      default: return PluginFactory::pluginName(plugin);
    }
  }

  // returns description of plugin
  char *pluginDesc(uint16_t plugin)
  {
    switch (plugin)
    {
      #if PLUGIN_SPECTRA
      case 70: return (char*)"Spectra reacts to sound.";
      #endif

      #if PLUGIN_PLASMA
      case 80: return (char*)"Plasma is groovy.";
      #endif

      default: return PluginFactory::pluginDesc(plugin);
    }
  }

  // returns bits descripting capabilities (PNP_EBIT_ values)
  uint16_t pluginBits(uint16_t plugin)
  {
    switch (plugin)
    {
      #if PLUGIN_SPECTRA
      case 70: return PNP_EBIT_REDRAW;
      #endif

      #if PLUGIN_PLASMA
      case 80: return PNP_EBIT_REDRAW | PNP_EBIT_COUNT | PNP_EBIT_DELAY;
      #endif

      default: return PluginFactory::pluginBits(plugin);
    }
  }

  // returns true if plugin redraws, else filter
  bool pluginDraws(uint16_t plugin)
  {
    switch (plugin)
    {
      #if PLUGIN_SPECTRA
      case 70: return true;
      #endif

      #if PLUGIN_PLASMA
      case 80: return true;
      #endif

      default: return PluginFactory::pluginDraws(plugin);
    }
  }

  PixelNutPlugin *pluginCreate(uint16_t plugin)
  {
    switch (plugin)
    {
      #if PLUGIN_SPECTRA
      case 70: return new PNP_Spectra;
      #endif

      #if PLUGIN_PLASMA
      case 80: return new PNP_Plasma;
      #endif

      default: return PluginFactory::pluginCreate(plugin);
    }
  }
};

XPluginFactory xpluginFactory = XPluginFactory();
PluginFactory *pPluginFactory = &xpluginFactory;
#else
PluginFactory pluginFactory;
PluginFactory *pPluginFactory = &pluginFactory;

#endif // DEV_PLUGINS
