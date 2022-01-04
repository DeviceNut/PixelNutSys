// What Effect Does:
//
//    Modifies the current color hue/white properties using the the force value,
//    only when triggered. The amount of force determines how much it's modified.
//
// Calling trigger():
//
//    The larger the force, the more the color is pushed around the color wheel.
//    A force of 0 makes the minimum change possible. At full force the changes
//    are 1/10 of the maximum (36 degrees of hue and 10% of whiteness).
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
//    dvalueHue, pcentWhite - modified each call to trigger().
//

class PNP_ColorModify : public PixelNutPlugin
{
public:

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, byte force)
  {
    // apply current color adjusted with the force value
    float pcentforce = ((float)force / MAX_FORCE_VALUE);

    //pixelNutSupport.msgFormat(F("ColorModify1: force=%d%% hue=%d white=%d"),
    //    (int)(pcentforce*100), pdraw->dvalueHue, pdraw->pcentWhite);
 
    uint16_t addhue = (uint16_t)(pcentforce * MAX_DVALUE_HUE / 10);
    if (!addhue) addhue = 1;
    pdraw->dvalueHue += addhue;
    pdraw->dvalueHue %= (MAX_DVALUE_HUE + 1);

    uint16_t addwhite = (uint16_t)(pcentforce * MAX_PERCENTAGE / 10);
    if (!addwhite) addwhite = 1;
    pdraw->pcentWhite += addwhite;
    pdraw->pcentWhite %= 30; // keep under 30% white

    //pixelNutSupport.msgFormat(F("ColorModify2: force=%d%% hue=%d white=%d"),
    //    (int)(pcentforce*100), pdraw->dvalueHue, pdraw->pcentWhite);
 
    pixelNutSupport.makeColorVals(pdraw);
  }
};
