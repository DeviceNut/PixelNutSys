// What Effect Does:
//
//    Rotates color hue around the color wheel on each drawing step, but doesn't change the
//    whiteness or brightness. The amount of change that is made each time is determined by
//    the trigger force.
//
//    The more force the more degrees the hue changes each step. If force is 0 then the color
//    won't change at all. At maximum force the number of steps it takes to cycle through the
//    entire color wheel is exactly the same as the number of pixels.
//
//    Note that until this effect is triggered, the drawing color will stay red and won't change.
//
// Calling trigger():
//
//    Sets the degrees the color hue will change with each call to nextstep().
//
// Calling nextstep():
//
//    Sets the current drawing color and then advances the hue by some amount that was
//    determined by the force in the previous call to trigger().
//
// Properties Used:
//
//    percentWhite, percentBright - current values used to create the drawing color.
//
// Properties Affected:
//
//   dvalueHue - advanced by some number of degrees on each call to nextstep()
//

class PNP_HueRotate : public PixelNutPlugin
{
public:

  void begin(uint16_t id, uint16_t pixlen)
  {
    pixLength = pixlen;
    pixChanged = 0;
    curDegrees = 0.0;       // if trigger() not called then hue will be 0 (red)
    addDegrees = 0.0;       // which will not change until trigger() is called
    doResetAtEnd = false;
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, byte force)
  {
    // change hue by at most the number of degrees that "fit" exactly into the number of pixels
    addDegrees = (((float)force / MAX_FORCE_VALUE) * (MAX_DVALUE_HUE / (float)pixLength));

    if (force == MAX_FORCE_VALUE)
    {
      doResetAtEnd = true;
      pixChanged = 0;
    }
    else doResetAtEnd = false;

    //pixelNutSupport.msgFormat(F("HueRotate: force=%d pixlen=%d degrees=%.1f"), force, pixLength, addDegrees);
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    //pixelNutSupport.msgFormat(F("HueRotate: degrees=%d"), (int)curDegrees);

    pdraw->dvalueHue = (int)curDegrees;
    pixelNutSupport.makeColorVals(pdraw);

    if (doResetAtEnd && (++pixChanged >= pixLength))
    {
      pixChanged = 0;
      curDegrees = 0.0;
    }
    else
    {
      curDegrees += addDegrees;
      if (curDegrees > MAX_DVALUE_HUE) curDegrees = 0;
      else if (curDegrees < 0) curDegrees = MAX_DVALUE_HUE;
    }
  }

private:
  bool doResetAtEnd;
  uint16_t pixLength, pixChanged;
  float addDegrees, curDegrees;
};
