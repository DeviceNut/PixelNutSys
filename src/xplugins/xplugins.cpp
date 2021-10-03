/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"

#if DEV_PLUGINS

#include "PNP_Spectra.h"
#include "PNP_Plasma.h"

class XPluginFactory : public PluginFactory
{
public:
  PixelNutPlugin *makePlugin(int plugin)
  {
    switch (plugin)
    {
      #if PLUGIN_SPECTRA
      case 70: return new PNP_Spectra;
      #endif

      #if PLUGIN_PLASMA
      case 80: return new PNP_Plasma;
      #endif

      default: return PluginFactory::makePlugin(plugin);
    }
  }
};

XPluginFactory xpluginFactory = XPluginFactory();
PluginFactory *pPluginFactory = &xpluginFactory;
#else
PluginFactory pluginFactory;
PluginFactory *pPluginFactory = &pluginFactory;

#endif // DEV_PLUGINS
