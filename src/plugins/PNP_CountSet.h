// What Effect Does:
//
//    Directly sets the pixel count property from the force value on every trigger.
//
// Calling trigger():
//
//    Sets new pixel count value by scaling the force value, so that the maximum force sets
//    the maximum pixel count. A force of 0 is ignored.
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
//    pixCount - set each call to trigger().
//

class PNP_CountSet : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_TRIGGER | PLUGIN_TYPE_USEFORCE;
  };

  void begin(byte id, uint16_t pixlen)
  {
    pixLength = pixlen;
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    if (force != 0)
    {
      force = abs(force);
      pdraw->pixCount = pixelNutSupport.mapValue(force, 0, MAX_FORCE_VALUE, 1, pixLength);
    }
  }

private:
  uint16_t pixLength;
};
