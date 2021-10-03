// What Effect Does:
//
//    Scales brightness levels individually up and down to create a twinkle effect.
//    The number of pixels affected is determined by the pixel count property.
//    Allocates 2 bytes of memory per number of pixels.
//
// Calling trigger():
//
//    Not instantiated.
//
// Calling nextstep():
//
//    Sets some number of pixels to the current color with a calculated amount of
//    brightness, which varies up and down to create a twinkle effect.
//
// Properties Used:
//
//    r,g,b - the current color values.
//    pixCount - determines how many pixels are changed.
//
// Properties Affected:
//
//    none
//

class PNP_Twinkle : public PixelNutPlugin
{
public:
  ~PNP_Twinkle() { if (pbytes != NULL) free(pbytes); }

  byte gettype(void) const
  {
    return PLUGIN_TYPE_REDRAW;
  };

  void begin(byte id, uint16_t pixlen)
  {
    pixLength = pixlen;
    pbytes = (int16_t*)malloc(pixLength * sizeof(int16_t));

    maxvalue = 50;

    if (pbytes != NULL)
      for (uint16_t i = 0; i < pixLength; ++i)
        pbytes[i] = random(0, ((maxvalue * 2) + maxvalue)) - maxvalue;
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    if (pbytes == NULL) return;
    
    int draw = 0, skip = 0;

    for (uint16_t i = 0; i < pixLength; ++i)
    {
      if (!draw && !skip)
      {
        if (pdraw->pixCount == 1)
        {
          skip = 0;
          draw = pixLength;
        }
        else if (pdraw->pixCount == pixLength)
        {
          skip = pixLength;
          draw = 0;
        }
        else if (pdraw->pixCount > (pixLength - pdraw->pixCount))
        {
          skip = pdraw->pixCount / (pixLength - pdraw->pixCount);
          draw = 1;
        }
        else
        {
          draw = (pixLength - pdraw->pixCount) / pdraw->pixCount;
          skip = 1;
        }

        //pixelNutSupport.msgFormat(F("Twinkle: draw=%d skip=%d"), draw, skip);
      }

      float scale = 0.0;

      if (draw)
      {
        --draw;
        //pixelNutSupport.msgFormat(F("Twinkle: pbytes[%d]=%d"), i, pbytes[i]);

        bool doscale = true;

        if (pbytes[i] >= maxvalue)
        {
          if (--(pbytes[i]) >= maxvalue)
            continue; // keep off for now

          pbytes[i] = 1; // draw now, increasing level
        }
        else if (++(pbytes[i]) == 0)
        {
          pbytes[i] = maxvalue + random(10, 60); // go dark for random time
          doscale = false;
        }
        else if (pbytes[i] == maxvalue)
          pbytes[i] = -(maxvalue-1); // start decreasing level

        if (doscale) scale = ((float)abs(pbytes[i])) / maxvalue;
      }
      else
      {
        --skip;
        //pixelNutSupport.msgFormat(F("Twinkle: skipping #%d"), i);
      }

      pixelNutSupport.setPixel(handle, i, pdraw->r, pdraw->g, pdraw->b, scale);
    }
  }

private:
  uint16_t pixLength;
  int16_t *pbytes, maxvalue;
};
