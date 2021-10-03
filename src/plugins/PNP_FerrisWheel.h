// What Effect Does:
//
//    Draws evenly spaced pixels with the current color, shifting them
//    down one pixel at a time, creating a "ferris wheel" effect.
//
// Calling trigger():
//
//    Not instantiated.
//
// Calling nextstep():
//
//    Advances the effect by one pixel, by redrawing all of the pixels.
//
// Properties Used:
//
//    r,g,b - the current color values.
//    pixCount - determines the number of spokes in the "wheel".
//
// Properties Affected:
//
//    none
//

class PNP_FerrisWheel : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_REDRAW | PLUGIN_TYPE_DIRECTION;
  };

  void begin(byte id, uint16_t pixlen)
  {
    pixLength = pixlen;
    lastCount = 0;
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    if (lastCount != pdraw->pixCount)
    {
      lastCount = pdraw->pixCount;
      uint16_t spokeCount = lastCount;
      spaceCount = (pixLength - spokeCount);
      if (!spaceCount)
      {
        spaceCount = 1;
        spokeCount--;
      }
      spokeSpaces = spaceCount / spokeCount;
      if (spaceCount % spokeCount) ++spokeSpaces;
      spaceCount = 0;

      //pixelNutSupport.msgFormat(F("Ferris: count=%d spaces=%d"), spokeCount, spokeSpaces);
    }

    uint16_t count = spaceCount;

    for (uint16_t i = 0; i < pixLength; ++i) // draw all pixels
    {
      if (!count--)
      {
        count = spokeSpaces;
        pixelNutSupport.setPixel(handle, i, pdraw->r, pdraw->g, pdraw->b);
      }
      else pixelNutSupport.setPixel(handle, i, 0,0,0);
    }

    if (++spaceCount > spokeSpaces)
      spaceCount = 0;
  }

private:
  uint16_t pixLength, lastCount, spokeSpaces, spaceCount;
};
