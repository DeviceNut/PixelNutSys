// What Effect Does:
//
//    Modulates the original brightness value that has been set in the drawing property,
//    such that any affected animation appears to surge in brightness, then evenly dim
//    down until it is back to its original brightness.
//
//    This effect only happens after a trigger event, with the force determining the
//    amount the value is decreased with each surge. The number of steps it takes
//    is fixed to be 10 steps per the difference in brightness values.
//
//    For example: if the original value is set with 'B50', and the trigger force is
//    500 (max is 1000), then the surge sets the brightness value to 50 + 25
//    (500/1000*(100-50)), and it takes 250 (10*25) calls to 'nextstep()' to bring
//    the value back to 50 again.
//
//    Note: if the original value is 100 (the 'B' command is not used or set to 100),
//    then no modulation is possible and this plugin will have no effect at all.
//
// Calling trigger():
//
//    The first time this is called the min brightness is set to the current value.
//    In all cases the currrent brightness is set based on the amount of force applied:
//    at full force this is 100 (full brightness).
//
// Calling nextstep():
//
//    Decrements the brightness until it becomes the original value in the B command.
//
// Properties Used:
//
//    pcentBright - read to set the median brightness the very first call to nextstep().
//
// Properties Affected:
//
//    pcentBright - percentage of full brightness (0-100): set each call to nextstep().
//

class PNP_BrightSurge : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_TRIGGER | PLUGIN_TYPE_USEFORCE;
  };

  void begin(byte id, uint16_t pixlen)
  {
    minBright = -1;
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    if (minBright == (uint16_t)-1) minBright = pdraw->pcentBright; // set on very first trigger

    // map force directly to brightness
    force = abs(force);
    pdraw->pcentBright = pixelNutSupport.mapValue(force, 0, MAX_FORCE_VALUE, minBright, MAX_PERCENTAGE);
    stepCount = 0;

    //pixelNutSupport.msgFormat(F("BrightSurge: high=%d min=%d"), pdraw->pcentBright, minBright);
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    if (pdraw->pcentBright > minBright)
    {
      if (++stepCount/10)
      {
        --pdraw->pcentBright;
        stepCount = 0;

        pixelNutSupport.makeColorVals(pdraw);

        //pixelNutSupport.msgFormat(F("BrightSurge: percent %d => %d"), pdraw->pcentBright, minBright);
      }
    }
  }

private:
  uint16_t minBright;
  uint16_t stepCount;
};
