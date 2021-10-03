// What Effect Does:
//
//    Toggles the drawing direction property on each trigger.
//
// Calling trigger():
//
//    Switches direction between Up and Down.
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
//    goUpwards - current drawing direction.
//

class PNP_FlipDirection : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_TRIGGER;
  };

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    pdraw->goUpwards = !pdraw->goUpwards;
  }
};
