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
//    dvalueHue, pcentWhite - modified each call to nextstep().
//

class PNP_ColorRandom : public PixelNutPlugin
{
public:

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    pdraw->dvalueHue  = random(0, MAX_DVALUE_HUE+1);
    pdraw->pcentWhite = random(0, 60); // keep under 60% white
    pixelNutSupport.makeColorVals(pdraw);

    //pixelNutSupport.msgFormat(F("ColorRandom: hue=%d white=%d"), pdraw->dvalueHue, pdraw->pcentWhite);
  }
};
