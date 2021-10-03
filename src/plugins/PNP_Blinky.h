// What Effect Does:
//
//    Blinks on and off random pixels in the current color and brightness.
//    The number of pixels cleared/set each step is determined by the pixel
//    count property.
//
// Calling trigger():
//
//    Not instantiated.
//
// Calling nextstep():
//
//    Clears some pixels at random locations, then sets some pixels at random locations
//    to the colors currently set in the property for the current track.
//
// Properties Used:
//
//    r,g,b - the current color values.
//    pixCount - number of pixels set and cleared each nextstep().
//
// Properties Affected:
//
//    none
//

class PNP_Blinky : public PixelNutPlugin
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
    //pixelNutSupport.msgFormat(F("Blinky: pixcount=%d r=%d g=%d b=%d"), pdraw->pixCount, pdraw->r, pdraw->g, pdraw->b);

    // turn some off
    for (uint16_t i = 0; i < pdraw->pixCount; ++i)
    {
      uint16_t pos = random(0, pixLength);
      pixelNutSupport.setPixel(handle, pos, 0,0,0);
    }

    // turn some back on
    for (uint16_t i = 0; i < pdraw->pixCount; ++i)
    {
      uint16_t pos = random(0, pixLength);
      pixelNutSupport.setPixel(handle, pos, pdraw->r, pdraw->g, pdraw->b);
    }
  }

private:
  uint16_t pixLength;
};
