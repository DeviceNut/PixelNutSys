// What Effect Does:
//
//    Randomly sets pixels to the current color with a random brightness level.
//    The number of pixels set each step is determined by the pixel count property.
//
// Calling trigger():
//
//    Not instantiated.
//
// Calling nextstep():
//
//    Sets random pixels 'pixCount' times to a random brightness that's greater than 10%.
//
// Properties Used:
//
//    degreeHue, pcentWhite - determines the color.
//    pcentBright - determines the maximum brightness.
//    pixCount - number of pixels set each nextstep().
//
// Properties Affected:
//
//    r,g,b - the current color values.
//

class PNP_Noise : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_REDRAW;
  };

  void begin(byte id, uint16_t pixlen)
  {
    pixLength = pixlen;
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    PixelNutSupport::DrawProps p;

    // use current hue and whiteness
    p.degreeHue = pdraw->degreeHue;
    p.pcentWhite = pdraw->pcentWhite;

    for (uint16_t i = 0; i < pdraw->pixCount; ++i)
    {
      // set random brightness within limits (>= 10%)
      p.pcentBright = random(10, pdraw->pcentBright+1);
      pixelNutSupport.makeColorVals(&p);

      short pos = random(0, pixLength);
      pixelNutSupport.setPixel(handle, pos, p.r, p.g, p.b);
    }
  }

private:
  uint16_t pixLength;
};
