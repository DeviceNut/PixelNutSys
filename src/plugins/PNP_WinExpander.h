// What Effect Does:
//
//    Expands/contracts the drawing window continuously, centered on the
//    middle of the window.
//
// Calling trigger():
//
//    Saves force value to pass on when it reaches the maximum extent.
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

  void begin(uint16_t id, uint16_t pixlen)
  {
    myid = id;

    goForward = true; // start by expanding
    pixCenter = pixlen >> 1; // middle of strand

    headPos = tailPos = pixCenter;
    if (!(pixlen & 1)) --headPos;

    //pixelNutSupport.msgFormat(F("WinXpand: pixlen=%d head.tail=%d.%d"), pixlen, headPos, tailPos);
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    active = true;
    forceVal = force;
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    if (!active) return;

    int16_t count = pdraw->pixCount;
    if (count < 4) count = 4;

    pdraw->pixStart = headPos;
    pdraw->pixLen = tailPos - headPos;

    //pixelNutSupport.msgFormat(F("WinXpand: forward=%d count=%d head.tail=%d.%d"), goForward, count, headPos, tailPos);

    if (goForward) // expand
    {
      if (headPos <= (pixCenter - (count >> 1)))
      {
        pixelNutSupport.sendForce(handle, myid, forceVal);
        if (pdraw->noRepeating) active = false;
        goForward = false;
      }
    }
    else // contract
    {
      if ((headPos == pixCenter) || (tailPos == pixCenter))
      {
        pixelNutSupport.sendForce(handle, myid, forceVal);
        if (pdraw->noRepeating) active = false;
        goForward = true;
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
  uint16_t myid;
  short forceVal;
  bool active;
  bool goForward;
  int16_t pixCenter, headPos, tailPos;
};
