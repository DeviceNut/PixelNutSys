// What Effect Does:
//
//    Directly sets the current color hue from the force value when triggered. As the force increases
//    the color hue changes from red->green->blue. A value of 0 sets the color to red, as does the maximum.
//
//    The whiteness and brightness are unaffected.
//
// Calling trigger():
//
//    Sets the color hue with the force value scaled to degrees.
//
// Calling nextstep():
//
//    Not instantiated.
//
// Properties Used:
//
//    none
//
// Properties Affected:
//
//    dvalueHue - set to a value determined by the trigger force.
//    r,g,b - current drawing color is set using new value of dvalueHue.
//

class PNP_HueSet : public PixelNutPlugin
{
public:

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, byte force)
  {
    pdraw->dvalueHue = (uint16_t)(((float)force / MAX_FORCE_VALUE) * MAX_DVALUE_HUE);

    pixelNutSupport.makeColorVals(pdraw);

    //pixelNutSupport.msgFormat(F("SetTheHue: hue=%d"), pdraw->dvalueHue);
  }
};
