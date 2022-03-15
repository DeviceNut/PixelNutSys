// What Effect Does:
//
//    Using the built-in comet handling functions, creates one or more comets (up to 12),
//    such that they either loop around the drawing window continuously, or disappear as 
//    they "fall off" of the end of the window.
//
//    A "comet" is a series of pixels drawn with the current color properties that moves
//    down the drawing window, with the brighness highest at the "head", decreasing evenly
//    to the "tail", thus creating the appearance of a "comet streaking through sky".
//
// Calling trigger():
//
//    Starts a new comet, either a one-shot or continuous depending on the property.
//
// Sending a trigger:
//
//    A trigger is generated only for comets that are not repeated, and is done when
//    it "falls off" the end of the strip.
//
// Calling nextstep():
//
//    Advances all of the comets currently created by one pixel.
//
// Properties Used:
//
//   dvalueHue, pcentWhite - determines the color of the comet body.
//   pcentBright - starting comet head brightness, fades down tail.
//   pixLength - length of the comet body.
//
// Properties Affected:
//
//    none
//

#include "core/PixelNutComets.h"    // support class for the comet effects

class PNP_CometHeads : public PixelNutPlugin
{
public:
  ~PNP_CometHeads() { pixelNutComets.cometHeadDelete(cdata); }

  void begin(uint16_t id, uint16_t pixlen)
  {
    pixLength = pixlen;
    myid = id;

    uint16_t maxheads = pixLength / 8; // one head for every 8 pixels up to 12
    if (maxheads < 1) maxheads = 1; // but at least one
    else if (maxheads > 12) maxheads = 12;

    cdata = pixelNutComets.cometHeadCreate(maxheads);
    if ((cdata == NULL) && (maxheads > 1)) // try for at least 1
      cdata = pixelNutComets.cometHeadCreate(1);

    //pixelNutSupport.msgFormat(F("CometHeads: maxheads=%d cdata=0x%08X"), maxheads, cdata);

    headCount = 0; // no heads drawn yet
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, byte force)
  {
    headCount = pixelNutComets.cometHeadAdd(cdata, !pdraw->noRepeating, pixLength);
    forceVal = force;

    //pixelNutSupport.msgFormat(F("CometHeads: heads=%d dorepeat=%d force=%d"),
    //  headCount, pdraw->continuous, force);
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    uint16_t count = pixelNutComets.cometHeadDraw(cdata, pdraw, handle, pixLength);
    if (count != headCount)
    {
      pixelNutSupport.sendForce(handle, myid, forceVal);
      headCount = count;
    }
  }

private:
  uint16_t myid;
  byte forceVal;
  uint16_t pixLength, headCount;
  PixelNutComets::cometData cdata;
};
