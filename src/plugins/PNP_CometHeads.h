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
//    The very first time this is called determines the mode:
//
//    1) The original force is 0: nothing happens the first time. Subsequent calls
//       cause a comet to be created but not repeated only if the force >= 0.
//       This mode allows for one-shot comets if new forces are positive.
//
//    2) The original force is !0: The first time a comet is created and repeated.
//       Subsequent calls will create a comet, which is repeated if the force >= 0.
//       This mode allows for additional repetitive comets if new forces are positive.
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
//   degreeHue, pcentWhite - determines the color of the comet body.
//   pcentBright - starting comet head brightness, fades down tail.
//   pixLength - length of the comet body.
//
// Properties Affected:
//
//    none
//

#include "PixelNutComets.h"    // support class for the comet effects

class PNP_CometHeads : public PixelNutPlugin
{
public:
  ~PNP_CometHeads() { pixelNutComets.cometHeadDelete(cdata); }

  byte gettype(void) const
  {
    return PLUGIN_TYPE_REDRAW   | PLUGIN_TYPE_DIRECTION |
           PLUGIN_TYPE_TRIGGER  | PLUGIN_TYPE_NEGFORCE  |
           PLUGIN_TYPE_USEFORCE | PLUGIN_TYPE_SENDFORCE;
  };

  void begin(byte id, uint16_t pixlen)
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
    firstime = true;
  }

  void trigger(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw, short force)
  {
    bool doit = true;
    bool dorepeat = true;

    if (firstime)
    {
      if (force == 0)
      {
        doit = false;
        repMode = false;
      }
      else repMode = true;

      firstime = false;
    }
    else if (repMode) dorepeat = (force >= 0);
    else if (force >= 0) dorepeat = false;
    else doit = false;

    //pixelNutSupport.msgFormat(F("CometHeads: force=%d doit=%d dorepeat=%d mode=%d"), force, doit, dorepeat, repMode);

    if (doit) headCount = pixelNutComets.cometHeadAdd(cdata, myid, dorepeat, pixLength);
    forceVal = force;
  }

  void nextstep(PixelNutHandle handle, PixelNutSupport::DrawProps *pdraw)
  {
    uint16_t count = pixelNutComets.cometHeadDraw(cdata, myid, pdraw, handle, pixLength);
    if (count != headCount)
    {
      pixelNutSupport.sendForce(handle, myid, forceVal, pdraw);
      headCount = count;
    }
  }

private:
  byte myid;
  bool firstime, repMode;
  short forceVal;
  uint16_t pixLength, headCount;
  PixelNutComets::cometData cdata;
};
