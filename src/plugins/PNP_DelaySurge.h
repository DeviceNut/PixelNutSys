// What Effect Does:
//
//    Modulates the delay percent property such that any affected animation appears
//    to surge in speed, then evenly slow down until it is back to its original speed.
//
//    This effect only happens after a trigger event, with the force determining the
//    amount the delay is decreased with each surge. The number of steps it takes
//    is fixed to be 10 * difference in delay values (10 steps per delay value used).
//
//    For example: if the original value is set with 'D20', and the trigger force is
//    127 (max is 255), then the surge sets the delay value to 10 ((127/255)*20), and
//    it takes 100 (10*10) calls to 'nextstep()' to bring the value back to 20 again.
//
//    Note: if the original delay value is 0 this plugin has no effect.
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
//    pcentDelay - sets the maximum delay the very first call to nextstep().
//
// Properties Affected:
//
//    pcentDelay - delay time in milliseconds: set each call to nextstep().
//

class PNP_DelaySurge : public PixelNutPlugin
{
public:

  void begin(uint16_t id, uint16_t pixlen)
  {
    maxDelay = -1;
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, byte force)
  {
    if (maxDelay == (uint16_t)-1) maxDelay = pdraw->pcentDelay; // set on very first trigger

    // map inverse force between 0 and the max value (more force is less delay)
    pdraw->pcentDelay = pixelNutSupport.mapValue(force, 0, MAX_FORCE_VALUE, maxDelay, 0);
    stepCount = 0;

    //pixelNutSupport.msgFormat(F("DelaySurge: low=%d max=%d"), pdraw->pcentDelay, maxDelay);
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    if (pdraw->pcentDelay < maxDelay)
    {
      if (++stepCount/10)
      {
        ++pdraw->pcentDelay;
        stepCount = 0;

        //pixelNutSupport.msgFormat(F("DelaySurge: delay %d => %d"), pdraw->pcentDelay, maxDelay);
      }
    }
  }

private:
  uint16_t maxDelay;
  uint16_t stepCount;
};
