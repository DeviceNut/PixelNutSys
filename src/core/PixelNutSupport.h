/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#pragma once

#define ATTR_PACKED __attribute__ ((packed))
#define C_ASSERT(x) extern "C" int __CPP_ASSERT__ [(x)?1:-1]

// useful physical constants:
#define PI_VALUE            (3.1415)
#define RADIANS_PER_CIRCLE  (2 * PI_VALUE)  // radians in a circle
#define DEGREES_PER_CIRCLE  360             // degrees in a circle

// maximum values for properties:
#define MAX_PERCENTAGE            100     // max percent value (0..100)
#define DEF_PERCENTAGE            50      // default percentage (brightness/delay)
#define MAX_PIXEL_VALUE           255     // max value for pixel
#define MAX_LAYER_VALUE           255     // max number of layers in pattern
#define MAX_DVALUE_HUE            359     // max value for hue
#define MAX_FORCE_VALUE           255     // max value for force
#define MAX_PLUGIN_VALUE          32000   // max value for plugin

typedef void* PixelNutHandle;   // context to call methods with

typedef uint32_t (*GetMsecsTime)(void);

typedef struct // defines ordering of RGB pixel values
{
  byte r,g,b;
}
PixelValOrder;

class PixelNutSupport // Support definitions and services for PixelNut engine and applications
{
public:
  // 'pix_array' is array of 3 bytes: index of Red, Green, Blue pixels
  // WS2812B pixels are ordered GRB: array = [1,0,2]
  // APA102 pixels are ordered BGR, array [2,1,0]
  PixelNutSupport(GetMsecsTime get_msecs, PixelValOrder *pix_order); // constructor

  /////////////////////////////////////////////////////////////////////////////
  // Optionally set from the main application; used by the PixelNut Engine.
  // The Plugins use the debug message output formating call as well.

  // abstracts interface to get milliseconds count since bootup
  GetMsecsTime getMsecs;

  // abstracts interface from debug output display formatting
  void (*msgFormat)(const char *str, ...);

  /////////////////////////////////////////////////////////////////////////////
  // The rest of the interface is used by the Engine to call into the Plugins,
  // and the Plugins to draw into pixel buffers and handle trigger events.

  // properties that can be modified at any time by commands/plugins:
  typedef struct ATTR_PACKED // 18 bytes
  {
    uint16_t pixStart;          // start of pixel draw range (from 0)
    uint16_t pixLen;            // length of pixels to drawn (1-max)
    uint16_t pixCount;          // pixel count property (from 1)

    uint16_t dvalueHue;         // hue in degrees (0-MAX_DVALUE_HUE)
    byte pcentBright;           // percent brightness (0-MAX_PERCENTAGE)
    byte pcentDelay;            // determines delay after each redraw
    byte pcentWhite;            // percent whiteness (0-MAX_PERCENTAGE)
    byte r,g,b;                 // RGB calculated from (hue,white,bright)

    bool goBackwards;           // direction of drawing (true for end-to-start)
    bool pixOrValues;           // whether pixels overwrite or are combined
    bool noRepeating;           // true for one-shot, else continuous

    uint8_t filler;
  }
  DrawProps; // defines properties used in drawing an effect

  void makeColorVals(DrawProps *pdraw); // performs translation of hue/white/bright to RGB pixel values

  // abstracts plugins from the direct handling of the pixel values:
  void movePixels( PixelNutHandle p, uint16_t startpos, uint16_t endpos, uint16_t newpos);    // moves range of pixels
  void clearPixels(PixelNutHandle p, uint16_t startpos, uint16_t endpos);                     // clears range of pixels
  void getPixel(   PixelNutHandle p, uint16_t pos, byte *ptr_r, byte *ptr_g, byte *ptr_b);    // gets RGB pixel values
  void setPixel(   PixelNutHandle p, uint16_t pos, byte r, byte g, byte b, float scale=1.0);  // sets RGB pixel values
  void setPixel(   PixelNutHandle p, uint16_t pos, float scale); // scales existing value without applying gamma correction

  // utility functions to map and clip values into/over a range of values
  long mapValue(long inval, long in_min, long in_max, long out_min, long out_max);
  long clipValue(long inval, long out_min, long out_max);

  // sends trigger force to any other effect that has been assigned to this 'id'
  void sendForce(PixelNutHandle p, uint16_t id, byte force);
};

extern PixelNutSupport pixelNutSupport; // single statically allocated instance
