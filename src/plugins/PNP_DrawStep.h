// What Effect Does:
//
//    Draws one pixel at a time using the current color properties, advancing one pixel
//    down the strip each call to nextstep().
//
// Calling trigger():
//
//    Saves force value to pass on when it reaches the end.
//
// Calling nextstep():
//
//    Draws a single pixel each time, wrapping around the strip when the end is reached.
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

class PNP_DrawStep : public PixelNutPlugin
{
public:

  void begin(uint16_t id, uint16_t pixlen)
  {
    myid = id;
    pixLength = pixlen;
    curPos = 0;
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    forceVal = force;
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    //pixelNutSupport.msgFormat(F("DrawStep: curpos=%d, r=%d, g=%d, b=%d"), curPos, pdraw->r, pdraw->g, pdraw->b);

    pixelNutSupport.setPixel(handle, curPos, pdraw->r, pdraw->g, pdraw->b);

    if (++curPos >= pixLength)
    {
      curPos = 0;
      pixelNutSupport.sendForce(handle, myid, forceVal);
    }
  }

private:
  uint16_t myid;
  short forceVal;
  uint16_t pixLength, curPos;
};
