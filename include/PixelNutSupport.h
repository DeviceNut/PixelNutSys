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
#define RADIANS_PER_WAVE    (2 * PI_VALUE)  // radians in a circle

// maximum values for properties:
#define MAX_BYTE_VALUE            255     // max value in 8 bits (unsigned)
#define MAX_WORD_VALUE            65535   // max value in 16 bits (unsigned)
#define MAX_PERCENTAGE            100     // max percent value (0..100)
#define MAX_DEGREES_HUE           359     // hue value is 0-359
#define MAX_TRACK_LAYER           254     // max value for track/layer
#define MAX_PIXEL_VALUE           255     // max value for pixel
#define MAX_DELAY_VALUE           255     // max value for delay
#define MAX_FORCE_VALUE           1000    // max value for force
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
  typedef struct ATTR_PACKED // 14 bytes
  {
      uint16_t pixStart, pixLen;  // start/length of range of pixels to be drawn (0...)
      uint16_t pixCount;          // pixel count property, not related to above extent

      uint16_t degreeHue;         // hue in degrees (0-MAX_DEGREES_HUE)
      byte pcentWhite;            // percent whiteness (0-MAX_PERCENTAGE)
      byte pcentBright;           // percent brightness (0-MAX_PERCENTAGE)
      byte r,g,b;                 // RGB calculated from the above 3 values

      byte msecsDelay;            // determines msecs delay after each redraw

      bool goUpwards;             // direction of drawing (pixel index)
      bool orPixelValues;         // whether pixels overwrites or are OR'ed
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
  void sendForce(PixelNutHandle p, byte id, short force, DrawProps *pdraw);
};

extern PixelNutSupport pixelNutSupport; // single statically allocated instance
