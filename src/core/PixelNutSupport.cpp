// PixelNut Support Interface Class Implementation
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#include "core.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal helper routines
////////////////////////////////////////////////////////////////////////////////////////////////////

// modified from a standard gamma distribution to be non-zero in the lowest entries except at 0
static const byte gamma_vals[] =
{
  0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, // 0x00-0x0F
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x10-0x1F
  2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, // 0x20-0x2F
  3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, // 0x30-0x3F
  5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, // 0x40-0x4F
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16, // 0x50-0x5F
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25, // 0x60-0x6F
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36, // 0x70-0x7F
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50, // 0x80-0x8F
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68, // 0x90-0x9F
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89, // 0xA0-0xAF
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114, // 0xB0-0xBF
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142, // 0xC0-0xCF
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175, // 0xD0-0xDF
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213, // 0xE0-0xEF
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255  // 0xF0-0xFF
};

static byte GammaCorrection(byte inval) { return pgm_read_byte(&gamma_vals[inval]); }

// hue: 0...MAX_DVALUE_HUE
// sat: 0...MAX_PERCENTAGE
// val: 0...MAX_PERCENTAGE
static void HSVtoRGB(int hue, byte sat, byte val, byte *rptr, byte *gptr, byte *bptr)
{
  // convert brightness value from percentage to a byte value
  //val = GammaCorrection(val); // gamma correction to brightness

  float s = (float)sat / MAX_PERCENTAGE;
  float v = (float)val / MAX_PERCENTAGE;
  float h = (float)hue / (MAX_DVALUE_HUE+1);
  float q = (h * 360) / 60; // which 60 degree section
  float smod = q - (int)q; // saturation modifier 0..1
  float r, g, b; // values 0..1

  switch ((int)q)
  {
  case 0: // 0-60
  {
      r = v;
      g = v * (1 - (s * (1 - smod)));
      b = v * (1 - s);
      break;
  }
  case 1: // 60-120
  {
      r = v * (1 - (s * smod));
      g = v;
      b = v * (1 - s);
      break;
  }
  case 2: // 120-180
  {
      r = v * (1 - s);
      g = v;
      b = v * (1 - (s * (1 - smod)));
      break;
  }
  case 3: // 180-240
  {
      r = v * (1 - s);
      g = v * (1 - (s * smod));
      b = v;
      break;
  }
  case 4: // 240-300
  {
      r = v * (1 - (s * (1 - smod)));
      g = v * (1 - s);
      b = v;
      break;
  }
  default:
  case 5: // 300-359
  {
      r = v;
      g = v * (1 - s);
      b = v * (1 - (s * smod));
      break;
  }
  }

  *rptr = GammaCorrection(r * MAX_PIXEL_VALUE);
  *gptr = GammaCorrection(g * MAX_PIXEL_VALUE);
  *bptr = GammaCorrection(b * MAX_PIXEL_VALUE);
}

static PixelValOrder *pPixOrder;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public interface routines
////////////////////////////////////////////////////////////////////////////////////////////////////

PixelNutSupport::PixelNutSupport(GetMsecsTime get_msecs, PixelValOrder *pix_order) // constructor
{
  pPixOrder = pix_order;  // sets ordering of pixel RGB
  getMsecs = get_msecs;   // sets routine to get time
  msgFormat = MsgFormat;  // default is no debug output
}

void PixelNutSupport::makeColorVals(DrawProps *pdraw)
{
  HSVtoRGB(pdraw->dvalueHue, (MAX_PERCENTAGE - pdraw->pcentWhite), pdraw->pcentBright,
            &pdraw->r, &pdraw->g, &pdraw->b);
}

void PixelNutSupport::movePixels(PixelNutHandle handle, uint16_t startpos, uint16_t endpos, uint16_t newpos)
{
  PixelNutEngine *pEngine = (PixelNutEngine*)handle;
  if (pEngine->pDrawPixels != NULL)
  {
    byte *ppixs1 = (pEngine->pDrawPixels + (startpos * 3));
    byte *ppixs2 = (pEngine->pDrawPixels + (newpos * 3));
    int count = (endpos - startpos + 1) * 3;
    memmove(ppixs2, ppixs1, count); 
  }
}

void PixelNutSupport::clearPixels(PixelNutHandle handle, uint16_t startpos, uint16_t endpos)
{
  PixelNutEngine *pEngine = (PixelNutEngine*)handle;
  if (pEngine->pDrawPixels != NULL)
  {
    byte *ppixs = (pEngine->pDrawPixels + (startpos * 3));
    int count = (endpos - startpos + 1) * 3;
    memset(ppixs, 0, count);
  }
}

void PixelNutSupport::getPixel(PixelNutHandle handle, uint16_t pos, byte *ptr_r, byte *ptr_g, byte *ptr_b)
{
  PixelNutEngine *pEngine = (PixelNutEngine*)handle;
  if (pEngine->pDrawPixels != NULL)
  {
    byte *ppixs = (pEngine->pDrawPixels + (pos * 3));
    *ptr_r = ppixs[pPixOrder->r];
    *ptr_g = ppixs[pPixOrder->g];
    *ptr_b = ppixs[pPixOrder->b];
  }
}

void PixelNutSupport::setPixel(PixelNutHandle handle, uint16_t pos, byte r, byte g, byte b, float scale)
{
  PixelNutEngine *pEngine = (PixelNutEngine*)handle;
  if (pEngine->pDrawPixels != NULL)
  {
    byte *ppixs = (pEngine->pDrawPixels + (pos * 3));

    byte brightval = (scale * pEngine->getBrightPercent() * MAX_PIXEL_VALUE) / MAX_PERCENTAGE;
    float factor = ((float)GammaCorrection(brightval) / MAX_PIXEL_VALUE);

    ppixs[pPixOrder->r] = r * factor;
    ppixs[pPixOrder->g] = g * factor;
    ppixs[pPixOrder->b] = b * factor;
  }
}

void PixelNutSupport::setPixel(PixelNutHandle handle, uint16_t pos, float scale)
{
  PixelNutEngine *pEngine = (PixelNutEngine*)handle;
  if (pEngine->pDrawPixels != NULL)
  {
    byte *ppixs = (pEngine->pDrawPixels + (pos * 3));

    ppixs[pPixOrder->r] *= scale;
    ppixs[pPixOrder->g] *= scale;
    ppixs[pPixOrder->b] *= scale;
  }
}

long PixelNutSupport::mapValue(long inval, long in_min, long in_max, long out_min, long out_max)
{
  long range = (in_max - in_min);
  long round = range / 2;
  return ((((inval - in_min) * (out_max - out_min)) + round) / range) + out_min;
}

long PixelNutSupport::clipValue(long inval, long out_min, long out_max)
{
  if (inval > out_max) return out_max;
  if (inval < out_min) return out_min;
  return inval;
}

void PixelNutSupport::sendForce(PixelNutHandle handle, uint16_t id, byte force)
{
  PixelNutEngine *pEngine = (PixelNutEngine*)handle;
  pEngine->triggerForce(id, force);
}
