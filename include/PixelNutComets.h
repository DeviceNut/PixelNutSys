// PixelNut Comet Effect Plugin Support Class Definition
// Used by effect plugins that use comets.
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#pragma once

// Routines for drawing comets effects:
// Create: assigns data space to hold requested heads, returns NULL if failed
// Delete: must be called by plugin destructor to clean up any memory allocated
// Add: creates new head, or overwrites old one if already reached the maximum
//      ('dowrap' controls whether or not comet wraps around, or falls off end)
// Draw: draws all heads given draw settings, returns true if anything drawn
//       (length of comet is controlled by "pixCount" parameter in DrawProps)
// Both Draw/Add return the number of heads currently in use

class PixelNutComets
{
public:
    typedef void (*cometData); // abstracts internal data used for heads
    cometData cometHeadCreate(int headcount);
    void cometHeadDelete(cometData cdata);
    int cometHeadAdd(cometData cdata, byte layer, bool dowrap, int pixlen);
    int cometHeadDraw(cometData cdata, byte layer,
          PixelNutSupport::DrawProps *pdraw, PixelNutHandle handle, int pixlen);
};

extern PixelNutComets pixelNutComets; // single statically allocated object instance
