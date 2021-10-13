/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#pragma once

// Only the destructor and the gettype functions must be implemented in a derived class
// (although to do anything interesting one of other functions needs to be overriden too).

#define PLUGIN_TYPE_REDRAW        0x01  // creates pixel values from settings, else
                                        // alters those effect settings before drawing

                                        // any combination of these is valid:
#define PLUGIN_TYPE_DIRECTION     0x08  // changing direction changes effect
#define PLUGIN_TYPE_TRIGGER       0x10  // triggering changes the effect
#define PLUGIN_TYPE_USEFORCE      0x20  // trigger force is used in effect
#define PLUGIN_TYPE_NEGFORCE      0x40  // negative trigger force is used
#define PLUGIN_TYPE_SENDFORCE     0x80  // sends trigger force to other plugins

class PixelNutPlugin
{
public:
  virtual ~PixelNutPlugin() = 0; // an empty default method is provided

  // Returns capability bits indicating how this plugin affects pixel values.
  // This is the only required method that must be implemented in each plugin.
  virtual byte gettype(void) const = 0; // one or more PLUGIN_TYPE_ values

  // Start this effect, given the number of pixels in the strip to be drawn.
  // If any memory is allocated here make sure it's freed in the class destructor.
  // The "id" value identifies this layer, and is used to trigger other plugins.
  virtual void begin(byte id, uint16_t pixlen) {}

  // Trigger a change to the effect with an amount of "force" to be applied.
  // Guaranteed to be called here first before any calls to nextstep().
  virtual void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force) {}

  // Perform the next step of an effect by this plugin using the current drawing
  // properties. The rate at which this is called depends on the delay property.
  virtual void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw) {}
};
