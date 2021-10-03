// What Effect Does:
//
//    Increasing force increases the pixel count property, which then slowly reverts
//    back to the base value, which is set from the orginal value when first triggered.
//    The number of steps it takes is 10 * the distance from the base value to the new
//    value determined by the force when triggered.
//
//    Note: if the original value has been set to 'C100' (full length), then no modulation
//    is possible and this plugin will have no effect at all.
//
// Calling trigger():
//
//    The first time this is called the base count is set to the current value.
//    Every time this is called the count value is increased based on the amount
//    force applied: at full force this is equal to the number of pixels.
//
// Calling nextstep():
//
//    Decrements the pixel count by 1 if the value is greater than the original value
//    as set from trigger(), else do nothing (wait for next trigger event).
//
// Properties Affected:
//
//    pixCount - count value from 1...number-of-pixels
//

class PNP_CountSurge : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_TRIGGER | PLUGIN_TYPE_USEFORCE;
  };

  void begin(byte id, uint16_t pixlen)
  {
    pixLength = pixlen;
    baseCount = 0; // causes set on next trigger
    stepCount = 0;
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    if (!baseCount) baseCount = pdraw->pixCount;

    pdraw->pixCount = pixelNutSupport.mapValue(abs(force), 0, MAX_FORCE_VALUE, baseCount, pixLength);

    //pixelNutSupport.msgFormat(F("CountSurge: base=%d count=%d"), baseCount, pdraw->pixCount);
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    if ((pdraw->pixCount > baseCount) && ++stepCount/10)
    {
      --pdraw->pixCount;
      stepCount = 0;
    }
  }

private:
  uint16_t pixLength, baseCount, stepCount;
};
