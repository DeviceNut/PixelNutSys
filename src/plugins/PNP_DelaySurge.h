// What Effect Does:
//
//    Modulates the delay time property that has been set in the drawing property,
//    such that any affected animation appears to surge in speed, then evenly slow
//    down until it is back to its original speed.
//
//    This effect only happens after a trigger event, with the force determining the
//    amount the delay value is decreased with each surge. The number of steps it takes
//    is fixed to be 10 * difference in delay values (10 steps per delay value used).
//
//    For example: if the original value is set with 'D20', and the trigger force is
//    500 (max is 1000), then the surge sets the delay value to 10 (500/1000*20), and
//    it takes 100 (10*10) calls to 'nextstep()' to bring the value back to 20 again.
//
//    Note: if the original value is 0 (the 'D' command is not used or set to 0), then
//    no modulation is possible and this plugin will have no effect at all.
//
// Calling trigger():
//
//    The first time this is called the max delay is set to the current value. Then
//    the current delay value is set based based on the amount of force applied: at
//    full force it becomes 0.
//
// Calling nextstep():
//
//    Increments the delay time until it becomes the original value in the D command.
//
// Properties Used:
//
//    msecsDelay - sets the maximum delay the very first call to nextstep().
//
// Properties Affected:
//
//    msecsDelay - delay time in milliseconds: set each call to nextstep().
//

class PNP_DelaySurge : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_TRIGGER | PLUGIN_TYPE_USEFORCE;
  };

  void begin(byte id, uint16_t pixlen)
  {
    maxDelay = -1;
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    if (maxDelay == (uint16_t)-1) maxDelay = pdraw->msecsDelay; // set on very first trigger

    // map inverse force between 0 and the max value (more force is less delay)
    force = abs(force);
    pdraw->msecsDelay = pixelNutSupport.mapValue(force, 0, MAX_FORCE_VALUE, maxDelay, 0);
    stepCount = 0;

    //pixelNutSupport.msgFormat(F("DelaySurge: low=%d max=%d"), pdraw->msecsDelay, maxDelay);
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    if (pdraw->msecsDelay < maxDelay)
    {
      if (++stepCount/10)
      {
        ++pdraw->msecsDelay;
        stepCount = 0;

        //pixelNutSupport.msgFormat(F("DelaySurge: delay %d => %d"), pdraw->msecsDelay, maxDelay);
      }
    }
  }

private:
  uint16_t maxDelay;
  uint16_t stepCount;
};
