// PixelNut Engine Class Implementation of Drawing functions
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file

#include "core.h"

void PixelNutEngine::setPropertyMode(bool enable)
{
  DBGOUT((F("Engine property mode: %s"), (enable ? "enabled" : "disabled")));
  externPropMode = enable;
}

void PixelNutEngine::setColorProperty(uint16_t hue_value, byte white_percent)
{
  externValueHue = pixelNutSupport.clipValue(hue_value, 0, MAX_DVALUE_HUE);
  externPcentWhite = pixelNutSupport.clipValue(white_percent, 0, MAX_PERCENTAGE);
}

void PixelNutEngine::setCountProperty(byte pixcount_percent)
{
  externPcentCount = pixelNutSupport.clipValue(pixcount_percent, 0, MAX_PERCENTAGE);
}

// internal: override property values with external ones
void PixelNutEngine::OverridePropVals(PluginTrack *pTrack)
{
  DBGOUT((F("Override Track Properties:")));

  // map value into a pixel count (depends on the actual number of pixels)
  uint16_t count = pixelNutSupport.mapValue(externPcentCount, 0, MAX_PERCENTAGE, 1, numPixels);

  // adjust all tracks that allow extern control with Q command
  for (int i = 0; i <= indexTrackStack; ++i)
  {
    PluginTrack *pTrack = TRACK_MAKEPTR(i);

    if (pTrack->pLayer->mute) continue;

    bool doset = false;

    if (pTrack->ctrlBits & ExtControlBit_PixCount)
    {
      DBGOUT((F("  %d) cnt: %d => %d"), i, pTrack->draw.pixCount, count));
      pTrack->draw.pixCount = count;
    }

    if (pTrack->ctrlBits & ExtControlBit_DegreeHue)
    {
      DBGOUT((F("  %d) hue: %d => %d"), i, pTrack->draw.dvalueHue, externValueHue));
      pTrack->draw.dvalueHue = externValueHue;
      doset = true;
    }

    if (pTrack->ctrlBits & ExtControlBit_PcentWhite)
    {
      DBGOUT((F("  %d) wht: %d%% => %d%%"), i, pTrack->draw.pcentWhite, externPcentWhite));
      pTrack->draw.pcentWhite = externPcentWhite;
      doset = true;
    }

    if (doset) pixelNutSupport.makeColorVals(&pTrack->draw);
  }

}

// internal: restore property values to previous values
void PixelNutEngine::RestorePropVals(PluginTrack *pTrack, uint16_t pixCount, uint16_t dvalueHue, byte pcentWhite)
{
  if (pTrack->pLayer->mute) return;

  #if DEBUG_OUTPUT
  if (pTrack->ctrlBits & (ExtControlBit_PixCount | ExtControlBit_DegreeHue | ExtControlBit_PcentWhite))
    DBGOUT((F("Track=%d Restore Properties:"), TRACK_INDEX(pTrack)));
  #endif

  if ((pTrack->ctrlBits & ExtControlBit_PixCount) && (pTrack->draw.pixCount != pixCount))
  {
    DBGOUT((F("  cnt: %d => %d"), pTrack->draw.pixCount, pixCount));
    pTrack->draw.pixCount = pixCount;
  }

  bool doset = false;

  if ((pTrack->ctrlBits & ExtControlBit_DegreeHue) && (pTrack->draw.dvalueHue != dvalueHue))
  {
    DBGOUT((F("  hue: %d => %d"), pTrack->draw.dvalueHue, dvalueHue));
    pTrack->draw.dvalueHue = dvalueHue;
    doset = true;
  }

  if ((pTrack->ctrlBits & ExtControlBit_PcentWhite) && (pTrack->draw.pcentWhite != pcentWhite))
  {
    DBGOUT((F("  wht: %d => %d"), pTrack->draw.pcentWhite, pcentWhite));
    pTrack->draw.pcentWhite = pcentWhite;
    doset = true;
  }

  if (doset) pixelNutSupport.makeColorVals(&pTrack->draw);
}

bool PixelNutEngine::updateEffects(void)
{
  bool doshow = (msTimeUpdate == 0);

  uint32_t time = pixelNutSupport.getMsecs();
  bool rollover = (msTimeUpdate > time);
  msTimeUpdate = time;

  RepeatTriger(); //check if need to generate a trigger

  // first have any redraw effects that are ready draw into its own buffers...

  for (int i = 0; i <= indexTrackStack; ++i) // for each plugin that can redraw
  {
    PluginTrack *pTrack = TRACK_MAKEPTR(i);
    PluginLayer *pLayer = pTrack->pLayer;

    //DBGOUT((F("Update: id=%d trig=%d mute=%d"),
    //        pLayer->thisLayerID, pLayer->trigActive, pLayer->mute));

    if (!pLayer->trigActive) continue; // not triggered yet

    if (pLayer->mute) // track is muted, but still update with its pixels
    {
      doshow = true;
      continue;
    }

    // update the time if it's rolled over, then check if time to draw
    if (rollover) pTrack->msTimeRedraw = msTimeUpdate;
    if (pTrack->msTimeRedraw > msTimeUpdate) continue;

    pDrawPixels = NULL; // prevent drawing by filter effects

    // call all filter effects for this track if triggered and not disabled
    PluginLayer *pfilter = pLayer + 1;
    for (int j = 1; j < pTrack->lcount; ++j, ++pfilter)
      if (pfilter->trigActive && !pfilter->mute)
        pfilter->pPlugin->nextstep(this, &pTrack->draw);

    short pixCount = 0;
    uint16_t dvalueHue = 0;
    byte pcentWhite = 0;

    if (externPropMode)
    {
      pixCount = pTrack->draw.pixCount;
      dvalueHue = pTrack->draw.dvalueHue;
      pcentWhite = pTrack->draw.pcentWhite;
      OverridePropVals(pTrack);
    }

    // now the main drawing effect is executed for this track
    pDrawPixels = TRACK_BUFFER(pTrack); // switch to drawing buffer
    pLayer->pPlugin->nextstep(this, &pTrack->draw);
    pDrawPixels = pDisplayPixels; // restore to draw from display buffer

    if (externPropMode) RestorePropVals(pTrack, pixCount, dvalueHue, pcentWhite);

    short addmsecs = (((maxDelayMsecs * pcentDelay) / MAX_PERCENTAGE) *
                           pTrack->draw.pcentDelay) / MAX_PERCENTAGE;
    //DBGOUT((F("delay=%d (%d*%d*%d)"), addmsecs, maxDelayMsecs, pcentDelay, pTrack->draw.pcentDelay));
    if (addmsecs <= 0) addmsecs = 1; // must advance at least by 1 each time
    pTrack->msTimeRedraw = msTimeUpdate + addmsecs;

    doshow = true;
  }

  if (doshow)
  {
    // merge all buffers whether just redrawn or not if anyone of them changed
    memset(pDisplayPixels, 0, pixelBytes); // must clear display buffer first

    for (int i = 0; i <= indexTrackStack; ++i) // for each plugin that can redraw
    {
      PluginTrack *pTrack = TRACK_MAKEPTR(i);
      PluginLayer *pLayer = pTrack->pLayer;

      // don't show if layer is muted or not triggered yet,
      // but do draw if just not updated from above
      if (pLayer->mute || !pLayer->trigActive)
        continue;

      short pixlast = numPixels-1;
      short pixstart = firstPixel + pTrack->draw.pixStart;
      //DBGOUT((F("%d PixStart: %d == %d+%d"), pTrack->draw.goBackwards, pixstart, firstPixel, pTrack->draw.pixStart));
      if (pixstart > pixlast) pixstart -= (pixlast+1);

      short pixend = pixstart + pTrack->draw.pixLen - 1;
      //DBGOUT((F("%d PixEnd:  %d == %d+%d-1"), pTrack->draw.goBackwards, pixend, pixstart, pTrack->draw.pixLen));
      if (pixend > pixlast) pixend -= (pixlast+1);

      short pix = (pTrack->draw.goBackwards ? pixend : pixstart);
      short x = pix * numBytesPerPixel;
      short y = pTrack->draw.pixStart * numBytesPerPixel;

      byte *ppix = TRACK_BUFFER(pTrack);
      /*
      DBGOUT((F("Input pixels:")));
      for (int i = 0; i < numPixels; ++i)
        DBGOUT((F("  %d.%d.%d"), *p++, *p++, *p++));
      */
      while(true)
      {
        //DBGOUT((F(">> start.end=%d.%d pix=%d x=%d y=%d"), pixstart, pixend, pix, x, y));

        if (pTrack->draw.pixOrValues)
        {
          // combine contents of buffer window with actual pixel array
          pDisplayPixels[x+0] |= ppix[y+0];
          pDisplayPixels[x+1] |= ppix[y+1];
          pDisplayPixels[x+2] |= ppix[y+2];
        }
        else if ((ppix[y+0] != 0) ||
                 (ppix[y+1] != 0) ||
                 (ppix[y+2] != 0))
        {
          pDisplayPixels[x+0] = ppix[y+0];
          pDisplayPixels[x+1] = ppix[y+1];
          pDisplayPixels[x+2] = ppix[y+2];
        }

        if (!pTrack->draw.goBackwards)
        {
          if (pix == pixend) break;

          if (pix >= pixlast) // wrap around to start of strip
          {
            pix = x = 0;
          }
          else
          {
            ++pix;
            x += 3;
          }
        }
        else // going backwards
        {
          if (pix == pixstart) break;
  
          if (pix <= 0) // wrap around to end of strip
          {
            pix = pixlast;
            x = (pixlast * numBytesPerPixel);
          }
          else
          {
            --pix;
            x -= 3;
          }
        }

        if (y >= (pixlast * numBytesPerPixel)) y = 0;
        else y += 3;
      }
    }

    /*
    byte *p = pDisplayPixels;
    for (int i = 0; i < numPixels; ++i)
      DBGOUT((F("%d.%d.%d"), *p++, *p++, *p++));
    */
  }

  return doshow;
}
