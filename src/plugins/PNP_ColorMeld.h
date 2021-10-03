// What Effect Does:
//
//    Smoothly melds from one color (hue/white) into another whenever the current color
//    is changed. The brightness is unaffected.
//
// Calling trigger():
//
//    Saves the force value to pass on when the target color is reached.
//
// Calling nextstep():
//
//    Modifies the hue color property by 1 degree, and the white color property by
//    1/20 of a percent each time, until the target color is reached.
//
// Properties Used:
//
//    degreeHue, pcentWhite - determines the endpoint color.
//
// Properties Affected:
//
//    degreeHue, pcentWhite - set each call to nextstep().
//

class PNP_ColorMeld : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_TRIGGER | PLUGIN_TYPE_NEGFORCE | PLUGIN_TYPE_SENDFORCE;
  };

  void begin(byte id, uint16_t pixlen)
  {
    myid = id;
    endHue = endWhite = -2; // forces initialization
    curHue = curWhite = -1; // don't make equal to avoid initial sendForce()
    stepWhite = 0.0;
    forceVal = 0;
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    forceVal = force;
    curHue = -1; // immediately draw current color
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    // Send force before checking for the color, so that if this causes the color to change
    // (by being triggered from this force), it will be detected as the new endpoint, but
    // won't be drawn, which would cause a flash for one cycle until the color reverts back
    // to its original value.

    //pixelNutSupport.msgFormat(F("ColorStep1: hue=%d.%d, white=%d.%d"), curHue, endHue, curWhite, endWhite);

    if ((curHue == endHue) && (curWhite == endWhite))
      pixelNutSupport.sendForce(handle, myid, forceVal, pdraw);

    endHue = pdraw->degreeHue;
    endWhite = pdraw->pcentWhite;

    //pixelNutSupport.msgFormat(F("ColorStep2: hue=%d.%d, white=%d.%d"), curHue, endHue, curWhite, endWhite);

    if ((curHue == endHue) && (curWhite == endWhite))
      return; // nothing to do

    if (curHue < 0) // first time initialization: draw current color
    {
      curHue = pdraw->degreeHue;
      curWhite = pdraw->pcentWhite;
    }
    else
    {
           if (curHue < endHue) curHue = (curHue + 1) % MAX_DEGREES_HUE;
      else if (curHue > endHue) curHue -= 1;

      if (curWhite != endWhite)
      {
        stepWhite += 0.05;
        if (stepWhite >= 1.0)
        {
          if (curWhite < endWhite)
               curWhite++;
          else curWhite--;

          stepWhite = 0.0;
        }
      }
    }

    //pixelNutSupport.msgFormat(F("ColorStep: hue=%d=>%d, white=%d=>%d"), curHue, endHue, curWhite, endWhite);

    pdraw->degreeHue = curHue;
    pdraw->pcentWhite = curWhite;
    pixelNutSupport.makeColorVals(pdraw);

    // to detect changes
    pdraw->degreeHue = endHue;
    pdraw->pcentWhite = endWhite;
  }

private:
  byte myid;
  short forceVal;
  int16_t curHue, curWhite;
  int16_t endHue, endWhite;
  float stepWhite;
};
