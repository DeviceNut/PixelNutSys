// What Effect Does:
//
//    Pushes whatever is currently drawn down the drawing window, drawing the current color
//    at the head each call to nextstep(). When the end of the strip is reached, it then
//    starts clearing the pixels, creating a "rolling" effect.
//
// Calling trigger():
//
//    Saves the force value to pass on when it reaches the end, and causes a new cycle to begin
//    from the start of the strip. If the original force was <= 0 then the cycle is not repeated
//    when it reaches the end of the display window.
//
// Calling nextstep():
//
//    Shifts (pushes) all pixels by one, then draws a single pixel to position 0.
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

class PNP_DrawPush : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_REDRAW | PLUGIN_TYPE_DIRECTION | PLUGIN_TYPE_NEGFORCE | PLUGIN_TYPE_SENDFORCE;
  };

  void begin(byte id, uint16_t pixlen)
  {
    myid = id;
    pixLength = pixlen;
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    forceVal = force; // if <0 then drawing stops after cycle
    doDraw = true;
    curPos = 0;
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    //pixelNutSupport.msgFormat(F("DrawPush: dodraw=%d curpos=%d r=%d g=%d b=%d"),
    //                            dodraw, curPos, pdraw->r, pdraw->g, pdraw->b);

    if (curPos)
    {
      uint16_t endpos = (curPos < pixLength-1) ? curPos : curPos-1;
      pixelNutSupport.movePixels(handle, 0, endpos, 1); // shift down one
    }

    if (doDraw)
         pixelNutSupport.setPixel(handle, 0, pdraw->r, pdraw->g, pdraw->b);
    else pixelNutSupport.setPixel(handle, 0, 0,0,0);

    if (curPos < pixLength-1) ++curPos;

    else if (doDraw)
    {
      doDraw = false;
      curPos = 0;
      pixelNutSupport.sendForce(handle, myid, forceVal, pdraw);
    }
    else if (forceVal > 0)
    {
      doDraw = true;
      curPos = 0;
      pixelNutSupport.sendForce(handle, myid, forceVal, pdraw);
    }
  }

private:
  byte myid;
  bool doDraw;
  short forceVal;
  uint16_t pixLength, curPos;
};
