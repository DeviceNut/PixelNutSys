// What Effect Does:
//
//    Modulates the original brightness value that has been set in the drawing
//    properties with a cosine function, such that any affected animation track
//    will have its brightness increase and decrease, continuously.
//
//    The brightness property is modulated up/down from the original value by 30%
//    of full brightness, with the force determining how many steps are taken in a
//    cycle: at full force there are 100 steps to this cycle, increasing to the
//    point where at a force=0 there is no modulation at all.
//
//    So the greater the force the more quickly the associated animation has its
//    brightness change.
//
//    For example: if the original value is set with 'B70', and the trigger force
//    is 400 (max is 1000), then the brightness changes from 70 down to 40, then
//    up to 90, and down to 40 again, and so on, with 250 (1000/400 * 100) steps
//    to the full cycle.
//
// Calling trigger():
//
//    Sets the force used in each step, which is saved and passed on when the
//    brightness reaches a maximum.
//
// Calling nextstep():
//
//    Sets the value of the 'pcentBright' property each time, using the saved force
//    value to determine the modulation with a cosine function. The very first time
//    this is called the median brightness is set to the current property value.
//
// Properties Used:
//
//    pcentBright - read to set the median brightness the very first call to nextstep().
//
// Properties Affected:
//
//    pcentBright - percentage of full brightness (0-100): set each call to nextstep().
//

class PNP_BrightWave : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_DIRECTION |
           PLUGIN_TYPE_TRIGGER | PLUGIN_TYPE_USEFORCE  | PLUGIN_TYPE_SENDFORCE;
  };

  void begin(byte id, uint16_t pixlen)
  {
    myid = id;
    baseValue = 0;   // will be set on first call to nextstep()
    angleNext = PI_VALUE; // starting angle for minimal brightness
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    forceVal = abs(force);
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    if (!baseValue) baseValue = pdraw->pcentBright;

    int bright = baseValue + (30 * cos(angleNext));
    if (bright <= 0)       pdraw->pcentBright = 0;
    else if (bright > 100) pdraw->pcentBright = 100;
    else                   pdraw->pcentBright = bright;

    pixelNutSupport.makeColorVals(pdraw);

    //pixelNutSupport.msgFormat(F("BrightWave: force=%d bright=%d angle(*100)=%d"), forceVal, pdraw->pcentBright, (int)(angleNext*100));

    angleNext += (RADIANS_PER_WAVE / 100.0) * ((float)forceVal / MAX_FORCE_VALUE);

    if (angleNext > RADIANS_PER_WAVE)
    {
      angleNext -= RADIANS_PER_WAVE;
      pixelNutSupport.sendForce(handle, myid, forceVal, pdraw);
    }
    else if (angleNext < 0)
      angleNext += RADIANS_PER_WAVE;
  }

private:
  byte myid;
  short forceVal;
  uint16_t baseValue;
  float angleNext;
};
