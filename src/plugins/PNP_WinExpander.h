// What Effect Does:
//
//    Expands/contracts the drawing window continuously, centered on the
//    middle of the window.
//
// Calling trigger():
//
//    Saves force value to pass on when it reaches the maximum extent.
//    Sends the negative of the force at the minimum extent.
//
// Calling nextstep():
//
//    Changes the size of the drawing window by 1 pixel on both ends.
//
// Properties Used:
//
//    pixCount - current value when nextstep() called determines maximum window size.
//
// Properties Affected:
//
//    pixStart, pixLen - starting/ending pixel position of the drawing window.
//

class PNP_WinExpander : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_NEGFORCE | PLUGIN_TYPE_SENDFORCE;
  };

  void begin(byte id, uint16_t pixlen)
  {
    myid = id;
    forceVal = 0;
    goForward = true; // start by expanding

    pixCenter = pixlen >> 1; // middle of strand
    headPos = tailPos = pixCenter;
    if (!(pixlen & 1)) --headPos;

    pixelNutSupport.msgFormat(F("WinXpand: pixlen=%d head.tail=%d.%d"), pixlen, headPos, tailPos);
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    forceVal = force;
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    int16_t count = pdraw->pixCount;
    if (count < 4) count = 4;

    pdraw->pixStart = headPos;
    pdraw->pixLen = tailPos - headPos;

    //pixelNutSupport.msgFormat(F("WinXpand: forward=%d count=%d head.tail=%d.%d"), goForward, count, headPos, tailPos);

    if (goForward) // expand
    {
      if (headPos <= (pixCenter - (count >> 1)))
      {
        goForward = false;
        pixelNutSupport.sendForce(handle, myid, forceVal, pdraw);
      }
    }
    else // contract
    {
      if ((headPos == pixCenter) || (tailPos == pixCenter))
      {
        goForward = true;
        pixelNutSupport.sendForce(handle, myid, -forceVal, pdraw);
      }
    }

    if (goForward)
    {
      --headPos;
      ++tailPos;
    }
    else
    {
      ++headPos;
      --tailPos;
    }
  }

private:
  byte myid;
  short forceVal;
  bool goForward;
  int16_t pixCenter, headPos, tailPos;
};
