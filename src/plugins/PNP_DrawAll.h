// What Effect Does:
//
//    Draws all of the pixels in the drawing window to the current color.
//
// Calling trigger():
//
//    Not instantiated.
//
// Calling nextstep():
//
//    Draws all pixels to the same color.
//
// Properties Used:
//
//    pcentBright - the brightness.
//    r,g,b - the current color values.
//
// Properties Affected:
//
//    none
//

class PNP_DrawAll : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_REDRAW | PLUGIN_TYPE_DIRECTION;
  };

  void begin(byte id, uint16_t pixlen)
  {
    pixLength = pixlen;
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    for (uint16_t i = 0; i < pixLength; ++i)
      pixelNutSupport.setPixel(handle, i, pdraw->r, pdraw->g, pdraw->b);
  }

private:
  uint16_t pixLength;
};
