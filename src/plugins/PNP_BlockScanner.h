// What Effect Does:
//
//    Draws a block of pixels with the current color back and forth
//    across the drawing window.
//
// Calling trigger():
//
//    Saves force value to pass on when it reaches the end.
//    Sends the negative of the force when it reaches the beginning.
//
// Calling nextstep():
//
//    Moves the block one pixel up/down by drawing and clearing pixels.
//
// Properties Used:
//
//    r,g,b - the current color values.
//    pixCount - current value when nextstep() called determines size of the color block.
//
// Properties Affected:
//
//    none
//

class PNP_BlockScanner : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_REDRAW | PLUGIN_TYPE_SENDFORCE;
  };

  void begin(byte id, uint16_t pixlen)
  {
    pixLength = pixlen;
    myid = id;

    goForward = true; // start going forward
    lastCount = 0;
    headPos = 0;
    forceVal = 0;
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    forceVal = force;
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    int16_t count = pdraw->pixCount;
    if (count>= pixLength) --count; // need at least one pixel free

    if (headPos < 0) headPos = 0;
    int16_t tailpos = headPos + count - 1;
    if (tailpos > (pixLength-1))
    {
      tailpos = (pixLength-1);
      headPos = tailpos - count + 1;
      if (headPos < 0) headPos = 0;
    }

    //pixelNutSupport.msgFormat(F("Bscan: forward=%d count=%d head.tail=%d.%d"), goForward, count, headPos, tailpos);

    // clear previously drawn pixels that are now off the end of
    // the block, because the length has changed since last time
    if (lastCount > count)
    {
      // compensate for previous adjustment to headPos
      int16_t endpos = (headPos + lastCount-1);
      if (endpos > (pixLength-1)) endpos = (pixLength-1);
      else endpos += (goForward ? -1 : 1);

      for (int16_t i = tailpos; i <= endpos; ++i)
        pixelNutSupport.setPixel(handle, i, 0,0,0);
    }
    lastCount = count;

    for (int16_t i = headPos; i <= tailpos; ++i)
      pixelNutSupport.setPixel(handle, i, pdraw->r, pdraw->g, pdraw->b);

    if (goForward)
    {
      if (headPos > 0) pixelNutSupport.setPixel(handle, (headPos-1), 0,0,0);

      if (tailpos >= (pixLength-1))
      {
        goForward = false;
        pixelNutSupport.sendForce(handle, myid, forceVal, pdraw);
      }
    }
    else
    {
      if (tailpos < (pixLength-1)) pixelNutSupport.setPixel(handle, (tailpos+1), 0,0,0);

      if (headPos <= 0)
      {
        goForward = true;
        pixelNutSupport.sendForce(handle, myid, -forceVal, pdraw);
      }
    }

    if (goForward) ++headPos;
    else          --headPos;
  }

private:
  byte myid;
  short forceVal;
  bool goForward;
  int16_t pixLength, lastCount, headPos;
};
