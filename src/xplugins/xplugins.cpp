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
// effect capability bits
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

class XPluginFactory : public PluginFactory
{
public:

  // returns bits descripting capabilities (PNP_EBIT_ values)
  bool pluginBits(int plugin)
  {
    switch (plugin)
    {
      #if PLUGIN_SPECTRA
      case 70: return 0;
      #endif

      #if PLUGIN_PLASMA
      case 80: return PNP_EBIT_COUNT | PNP_EBIT_DELAY;
      #endif

      default: return PluginFactory::pluginBits(plugin);
    }
  }

  // returns true if plugin redraws, else filter
  bool pluginDraws(int plugin)
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

  PixelNutPlugin *pluginCreate(int plugin)
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
