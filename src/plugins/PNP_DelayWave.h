// What Effect Does:
//
//    Modulates the original delay value that has been set in the drawing properties
//    with a cosine function, such that any affected animation track appears to speed
//    up and then slow down, continuously.
//
//    The delay property is modulated from the original value down to 0 and back, with
//    the force determining how many steps are taken in one cycle: at full force there
//    are 100 steps to this cycle, and as the force is decreased the number of steps
//    increases, to the point where at force=0 there is no modulation at all.
//
//    So the greater the force the more often the associated animation speeds up.
//    And if the original value is 0 (the 'D' command is not used or is set to 0),
//    then this plugin has no effect at all.
//
//    For example: if the original value is set with 'D30', and the trigger force is
//    250 (max is 1000), then the delay value will change from 30, down to 0, and then
//    back to 30, with 400 (1000/250 * 100) steps to the full cycle.
//
// Calling trigger():
//
//    Sets the force used in each step.
//
// Calling nextstep():
//
//    Sets the value of the 'msecsDelay' property each time, using the saved force
//    value to determine the modulation with a cosine function. The very first time
//    this is called the maximum delay is set to the current property value.
//
// Properties Used:
//
//    msecsDelay - read to set the maximum delay the very first call to nextstep().
//
// Properties Affected:
//
//    msecsDelay - delay time in milliseconds: set each call to nextstep().
//

class PNP_DelayWave : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_TRIGGER | PLUGIN_TYPE_USEFORCE | PLUGIN_TYPE_SENDFORCE;
  };

  void begin(byte id, uint16_t pixlen)
  {
    myid = id;
    maxDelay = 0;     // will be set on first call to nextstep()
    angleNext = 0.0;  // starting angle
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    forceVal = abs(force);
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    if (!maxDelay) maxDelay = pdraw->msecsDelay;

    // scale delay from 0 to maxDelay:
    pdraw->msecsDelay = maxDelay/2 * (cos(angleNext) + 1.0);

    //pixelNutSupport.msgFormat(F("DelayWave: delay=%d angle(*100)=%d"), pdraw->msecsDelay, (int)(angleNext*100));

    angleNext += (RADIANS_PER_WAVE / 100.0) * ((float)forceVal / MAX_FORCE_VALUE);

    if (angleNext > RADIANS_PER_WAVE)
    {
      angleNext -= RADIANS_PER_WAVE;
      pixelNutSupport.sendForce(handle, myid, forceVal, pdraw);
    }
    else if (angleNext < 0)
    {
      angleNext += RADIANS_PER_WAVE;
      pixelNutSupport.sendForce(handle, myid, forceVal, pdraw);
    }
  }

private:
  byte myid;
  short forceVal;
  uint16_t maxDelay;
  float angleNext;
};
