// What Effect Does:
//
//    Directly sets the pixel count property from the force value on every trigger.
//
// Calling trigger():
//
//    Sets new pixel count value by scaling the force value, so that the maximum force sets
//    the maximum pixel count.
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

  void begin(uint16_t id, uint16_t pixlen)
  {
    pixLength = pixlen;
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, byte force)
  {
    pdraw->pixCount = pixelNutSupport.mapValue(force, 0, MAX_FORCE_VALUE, 1, pixLength);
  }

private:
  uint16_t pixLength;
};
