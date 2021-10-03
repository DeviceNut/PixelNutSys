// What Effect Does:
//
//    Sets both color hue/white properties to random values on each step.
//    The brightness is not affected.
//
// Calling trigger():
//
//    Not instantiated.
//
// Calling nextstep():
//
//    Sets the color to random values, keeping the white properties under 60%
//    to avoid having most of the resultant colors being essentially white.
//
// Properties Used:
//
//    none
//
// Properties Affected:
//
//    degreeHue, pcentWhite - modified each call to nextstep().
//

class PNP_ColorRandom : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return 0;
  };

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    pdraw->degreeHue  = random(0, MAX_DEGREES_HUE+1);
    pdraw->pcentWhite = random(0, 60); // keep under 60% white
    pixelNutSupport.makeColorVals(pdraw);

    //pixelNutSupport.msgFormat(F("ColorRandom: hue=%d white=%d"), pdraw->degreeHue, pdraw->pcentWhite);
  }
};
