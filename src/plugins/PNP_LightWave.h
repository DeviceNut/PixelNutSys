// What Effect Does:
//
//    Creates waves in the current color that move down the drawing window, by using a cosine function
//    that modifies the current brightness level such that it creates the appearance of a light wave.
//
//    There are 10 or more steps (pixels) to the wave, determined by the pixel count property: the
//    larger the count the longer (more pixels to) the wave.
//
// Calling trigger():
//
//    Not instantiated.
//
// Calling nextstep():
//
//    Draws each pixel, scaling the brightness up/down from the current value.
//
// Properties Used:
//
//    r,g,b - the current color values.
//    pixCount - determines how many steps are taken in each wave.
//
// Properties Affected:
//
//    none
//

class PNP_LightWave : public PixelNutPlugin
{
public:
  byte gettype(void) const
  {
    return PLUGIN_TYPE_REDRAW | PLUGIN_TYPE_DIRECTION;
  };

  void begin(byte id, uint16_t pixlen)
  {
    myid = id;
    pixLength = pixlen;
    angleNext = 0.0; // starting angle
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    uint16_t count = (pixLength - pdraw->pixCount + 1);
    float angle_step = (RADIANS_PER_WAVE / 10.0) * ((float)count / pixLength);
    float angle = angleNext;

    for (uint16_t i = 0; i < pixLength; ++i, angle += angle_step)
    {
      float scale = ((cos(angle) + 1.0) / 4.0) + 0.5; // scale from 50-100%
      pixelNutSupport.setPixel(handle, i, pdraw->r, pdraw->g, pdraw->b, scale);

      //pixelNutSupport.msgFormat(F("LightWave: scale=%3d%%, r=%d, g=%d, b=%d"), (int)(scale*100), pdraw->r, pdraw->g, pdraw->b);
    }
    //pixelNutSupport.msgFormat(F("LightWave: angleNext * 100 = %d"), (int)(angleNext * 100));

    angleNext -= angle_step; // subtracting causes "forward" motion
    if (angleNext < 0) angleNext += RADIANS_PER_WAVE;
  }

private:
  byte myid;
  uint16_t pixLength;
  float angleNext;
};
