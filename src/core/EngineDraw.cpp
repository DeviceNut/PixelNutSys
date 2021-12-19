// PixelNut Engine Class Implementation of Drawing functions
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file (must also set in main.h)

#include "core.h"
#include "PixelNutPlugin.h"
#include "PixelNutEngine.h"

void PixelNutEngine::setPropertyMode(bool enable)
{
  DBGOUT((F("Engine property mode: %s"), (enable ? "enabled" : "disabled")));
  externPropMode = enable;
}

void PixelNutEngine::SetPropColor(void)
{
  DBGOUT((F("Engine color properties for tracks:")));

  // adjust all tracks that allow extern control with Q command
  for (int i = 0; i <= indexTrackStack; ++i)
  {
    PluginTrack *pTrack = TRACK_MAKEPTR(i);
    if (pTrack->pLayer->disable) continue;

    bool doset = false;

    if (pTrack->ctrlBits & ExtControlBit_DegreeHue)
    {
      DBGOUT((F("  %d) hue: %d => %d"), i, pTrack->draw.degreeHue, externDegreeHue));
      pTrack->draw.degreeHue = externDegreeHue;
      doset = true;
    }

    if (pTrack->ctrlBits & ExtControlBit_PcentWhite)
    {
      DBGOUT((F("  %d) whiteness: %d%% => %d%%"), i, pTrack->draw.pcentWhite, externPcentWhite));
      pTrack->draw.pcentWhite = externPcentWhite;
      doset = true;
    }

    if (doset) pixelNutSupport.makeColorVals(&pTrack->draw);
  }
}

void PixelNutEngine::setColorProperty(short hue_degree, byte white_percent)
{
  externDegreeHue = pixelNutSupport.clipValue(hue_degree, 0, MAX_DEGREES_HUE);
  externPcentWhite = pixelNutSupport.clipValue(white_percent, 0, MAX_PERCENTAGE);
  if (externPropMode) SetPropColor();
}

void PixelNutEngine::SetPropCount(void)
{
  DBGOUT((F("Engine pixel count property for tracks:")));

  // adjust all tracks that allow extern control with Q command
  for (int i = 0; i <= indexTrackStack; ++i)
  {
    PluginTrack *pTrack = TRACK_MAKEPTR(i);
    if (pTrack->pLayer->disable) continue;

    if (pTrack->ctrlBits & ExtControlBit_PixCount)
    {
      uint16_t count = pixelNutSupport.mapValue(externPcentCount, 0, MAX_PERCENTAGE, 1, numPixels);
      DBGOUT((F("  %d) %d => %d"), i, pTrack->draw.pixCount, count));
      pTrack->draw.pixCount = count;
    }
  }
}

void PixelNutEngine::setCountProperty(byte pixcount_percent)
{
  // clip and map value into a pixel count, dependent on the actual number of pixels
  externPcentCount = pixelNutSupport.clipValue(pixcount_percent, 0, MAX_PERCENTAGE);
  if (externPropMode) SetPropCount();
}

// internal: restore property values for bits set for track
void PixelNutEngine::RestorePropVals(PluginTrack *pTrack,
                      uint16_t pixCount, uint16_t degreeHue, byte pcentWhite)
{
  if (pTrack->pLayer->disable) return;

  if (pTrack->ctrlBits & ExtControlBit_PixCount)
    pTrack->draw.pixCount = pixCount;

  bool doset = false;

  if ((pTrack->ctrlBits & ExtControlBit_DegreeHue) &&
      (pTrack->draw.degreeHue != degreeHue))
  {
    //DBGOUT((F(">>hue: %d->%d"), pTrack->draw.degreeHue, degreeHue));
    pTrack->draw.degreeHue = degreeHue;
    doset = true;
  }

  if ((pTrack->ctrlBits & ExtControlBit_PcentWhite) &&
      (pTrack->draw.pcentWhite != pcentWhite))
  {
    //DBGOUT((F(">>wht: %d->%d"), pTrack->draw.pcentWhite, pcentWhite));
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

  RepeatTriger(rollover); //check if need to generate a trigger

  if (!patternEnabled) return doshow;

  // first have any redraw effects that are ready draw into its own buffers...

  for (int i = 0; i <= indexTrackStack; ++i) // for each plugin that can redraw
  {
    PluginTrack *pTrack = TRACK_MAKEPTR(i);
    PluginLayer *pLayer = pTrack->pLayer;

    //DBGOUT((F("Update: id=%d trig=%d disable=%d"),
    //        pLayer->thisLayerID, pLayer->trigActive, pLayer->disable));

    if (!pLayer->trigActive) continue; // not triggered yet

    if (pLayer->disable) // track is muted, but still update with pixels
    {
      doshow = true;
      continue;
    }

    // update the time if it's rolled over, then check if time to draw
    if (rollover) pTrack->msTimeRedraw = msTimeUpdate;
    if (pTrack->msTimeRedraw > msTimeUpdate) continue;

    short pixCount = 0;
    short degreeHue = 0;
    byte pcentWhite = 0;

    // prevent predraw effect from overwriting properties if in extern mode
    if (externPropMode)
    {
      pixCount = pTrack->draw.pixCount;
      degreeHue = pTrack->draw.degreeHue;
      pcentWhite = pTrack->draw.pcentWhite;
    }

    pDrawPixels = NULL; // prevent drawing by filter effects

    // call all filter effects for this track if triggered and not disabled
    PluginLayer *pfilter = pLayer + 1;
    for (int j = 1; j < pTrack->lcount; ++j)
      if (pfilter->trigActive && !pfilter->disable)
        pfilter->pPlugin->nextstep(this, &pTrack->draw);

    if (externPropMode) RestorePropVals(pTrack, pixCount, degreeHue, pcentWhite);

    // now the main drawing effect is executed for this track
    pDrawPixels = TRACK_BUFFER(pTrack); // switch to drawing buffer
    pLayer->pPlugin->nextstep(this, &pTrack->draw);
    pDrawPixels = pDisplayPixels; // restore to draw from display buffer

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

      // don't show if layer is disabled or not triggered yet,
      // but do draw if just not updated from above
      if (pLayer->disable || !pLayer->trigActive) continue;

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
