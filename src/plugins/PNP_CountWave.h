// What Effect Does:
//
//    Modulates the pixel count property with a cosine function, such that any
//    affected animation track using the count value will have it lengthen and
//    shorten, continuously.
//
//    The count property is modulated up/down from the original value by half the
//    total number of pixels, with the force determining how many steps are taken
//    in a cycle: at full force there are 100 steps to this cycle, increasing to
//    the point where at a force=0 there is no modulation at all.
//
//    So the greater the force the more quickly the associated animation has its
//    pixel count value change.
//
//    For example: if the original value is set with 'C50', and there are 100 pixels
//    total, and the trigger force is 200 (max is 1000), then the pixel count value
//    changes from 50 down to 25, then up to 75, and down to 25 again, etc., with
//    500 (1000/200 * 100) steps to the full cycle.
//
// Calling trigger():
//
//    Sets the force used in each step.
//
// Calling nextstep():
//
//    Sets the value of the 'pixCount' property each time, using the saved force
//    value to determine the modulation with a cosine function. The very first time
//    this is called the median value is set to the current property value.
//
// Properties Used:
//
//    pixCount - used to set the median pixel count the very first call to nextstep().
//
// Properties Affected:
//
//    pixCount - pixel count: set each call to nextstep().
//

class PNP_CountWave : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_TRIGGER  | PLUGIN_TYPE_USEFORCE |
           PLUGIN_TYPE_NEGFORCE | PLUGIN_TYPE_SENDFORCE;
  };

  void begin(byte id, uint16_t pixlen)
  {
    myid = id;
    pixLength = pixlen; // total number of pixels
    baseValue = 0;      // will be set on first call to nextstep()
    angleNext = 0.0;    // starting angle
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    forceVal = force; // can be negative
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    if (!baseValue) baseValue = pdraw->pixCount;

    uint16_t count = baseValue + (pixLength/2 * cos(angleNext));
    if (count <= 0)             pdraw->pixCount = 1;
    else if (count > pixLength) pdraw->pixCount = pixLength;
    else                        pdraw->pixCount = count;

    //pixelNutSupport.msgFormat(F("CountWave: count=%d angle(*100)=%d"), pdraw->pixCount, (int)(angleNext*100));

    angleNext += (RADIANS_PER_WAVE / 100) * ((float)forceVal / MAX_FORCE_VALUE);

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
  short forceVal, baseValue;
  uint16_t pixLength;
  float angleNext;
};
