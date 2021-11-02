/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#pragma once

// Only the destructor and the gettype functions must be implemented in a derived class
// (although to do anything interesting one of other functions needs to be overriden too).

class PixelNutPlugin
{
public:
  virtual ~PixelNutPlugin() = 0; // an empty default method is provided

  // Start this effect, given the number of pixels in the strip to be drawn.
  // If any memory is allocated here make sure it's freed in the class destructor.
  // The "id" value identifies this layer, and is used to trigger other plugins.
  virtual void begin(uint16_t id, uint16_t pixlen) {}

  // Trigger a change to the effect with an amount of "force" to be applied.
  // Guaranteed to be called here first before any calls to nextstep().
  virtual void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force) {}

  // Perform the next step of an effect by this plugin using the current drawing
  // properties. The rate at which this is called depends on the delay property.
  virtual void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw) {}
};
