// What Effect Does:
//
//    Directly sets the delay time property from the force value when triggered. Increased
//    force reduces the delay, so that at maximum force the delay is set to 0.
//
// Calling trigger():
//
//    Sets the delay time property.
//
// Calling nextstep():
//
//    Not instantiated.
//
// Properties Used:
//
//    pixCount - current value is the counter for the number of pixels to clear and set.
//
// Properties Affected:
//
//   delayMsecs - set to a value determined by the force used in the previous call to trigger().
//
// Effect:
//
// Trigger:
//   Sets new delay time value by scaling the force value, so that
//   the maximum force sets the minimum delay (fastest speed).
//
// Parameters:
//

class PNP_DelaySet : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_TRIGGER | PLUGIN_TYPE_USEFORCE;
  };

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    force = abs(force);
    // invert values: larger forces reduce the delay time
    pdraw->msecsDelay = pixelNutSupport.mapValue(force, 0, MAX_FORCE_VALUE, MAX_BYTE_VALUE, 0);
  }
};
