// PixelNut Comet Effect Plugin Support Class Implementation
/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#define DEBUG_OUTPUT 0 // 1 enables debugging this file

#include "core.h"

#include "core/PixelNutComets.h"  // support class for the comet effects
PixelNutComets pixelNutComets;    // single statically allocated object instance

////////////////////////////////////////////////////////////////////////////////////////////////////
// Internally used defines and data structures
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct ATTR_PACKED
{
  bool dowrap;                  // true to allow wrapping, false to fall off end
  bool offend;                  // false until head wraps or has gone off the end
  int16_t curpos;               // current head position (from 0)
  int16_t maxlen;               // max length of tail (distance to next head)
  int16_t prevlen;              // previous tail length (used to clear if shortened)
}
CometHead;        // defines a head for the comet effect
C_ASSERT(sizeof(CometHead) == 8);

typedef struct ATTR_PACKED
{
  byte count;                   // number of heads that are supported
  byte inuse;                   // number of heads currently in use
  CometHead heads[0];           // internally allocated head data starts here
}
CometHeadData;   // defines data for list of heads for the comet effect
C_ASSERT(sizeof(CometHeadData) == 2);

////////////////////////////////////////////////////////////////////////////////////////////////////

PixelNutComets::cometData PixelNutComets::cometHeadCreate(int headcount)
{
  void *memptr;
  int memlen = 0;

  while(1)
  {
    memlen = (sizeof(CometHeadData) + (headcount * sizeof(CometHead)));
    memptr = malloc(memlen);
    if (memptr != NULL) break;

    DBGOUT ((F("Cannot allocate %d bytes for comet heads"), memlen));
    return NULL;
  }

  CometHeadData *pData = (CometHeadData*)memptr;
  pData->count = headcount;
  pData->inuse = 0;

  CometHead *phead = pData->heads;
  for (int i = 0; i < headcount; ++i, ++phead)
  {
    // clear to unused status
    phead->dowrap = false;
    phead->offend = true;
  }

  DBGOUT((F("Allocated %d bytes for %d comet heads"), memlen, headcount));
  return (PixelNutComets::cometData)pData;
}

void PixelNutComets::cometHeadDelete(PixelNutComets::cometData cdata)
{
  CometHeadData *pData = (CometHeadData*)cdata;
  if (pData != NULL)
  {
    DBGOUT((F("Freed data for %d comet heads: %d in use"), pData->count, pData->inuse));
    free(pData);
  }
}

// adds new head or overwrites existing one if no more room, returns number of heads currently in use
int PixelNutComets::cometHeadAdd(PixelNutComets::cometData cdata, bool dowrap, int pixlen)
{
  if (cdata == NULL) return 0;

  CometHeadData *pData = (CometHeadData*)cdata;
  CometHead *phead = pData->heads;
  int maxheads = pData->count;

  #if DEBUG_OUTPUT
  DBGOUT((F("AddHead: Count=%d/%d"), pData->inuse, maxheads));
  DBGOUT((F("  Before:")));
  for (int i = 0; i < maxheads; ++i)
  {
    if (phead[i].dowrap || !phead[i].offend)
         DBGOUT((F("  #%d: DoWrap=%d OffEnd=%d CurPos=%-3d MaxLen=%-3d"), i,
                phead[i].dowrap, phead[i].offend, phead[i].curpos, phead[i].maxlen));
    //else DBGOUT((F("  #%d: unused"), i));
  }
  #endif

  // find empty slot or one with minimum position:

  int index = 0;
  int minpos = pixlen;
  bool doinsert = false;

  for (int i = 0; i < maxheads; ++i)
  {
    if (!phead[i].dowrap && phead[i].offend) // unused slot
    {
      doinsert = true;
      ++pData->inuse;
      index = i;
      break;
    }

    if (phead[i].curpos == 0) // already have head at starting position
      return pData->inuse;

    if (minpos > phead[i].curpos)
    {
      minpos = phead[i].curpos;
      index = i;
    }
  }

  // return if no empty slots
  if (!doinsert) return pData->inuse;

  // now find the minimum position excluding one just chosen:

  int next_index = -1;
  int maxlen = pixlen;
  minpos = pixlen;

  for (int i = 0; i < maxheads; ++i)
  {
    if (i == index) continue; // skip new one

    if (phead[i].dowrap || !phead[i].offend) // is in use
    {
      if (minpos > phead[i].curpos)
      {
        minpos = phead[i].curpos;
        next_index = i;
      }
    }
  }

  // if there's a head in front of this new one,
  // adjust its length and set maxlen for new one:

  if (next_index >= 0)
  {
    if (!dowrap)
         maxlen = pixlen; // until new head comes after
    else if (doinsert)
         maxlen = (phead[next_index].maxlen - phead[next_index].curpos);
    else maxlen = (phead[index].maxlen - phead[index].curpos);

    // next head's length is exactly it's current position since new one starts at 0
    phead[next_index].maxlen = phead[next_index].curpos;
  }

  phead[index].dowrap  = dowrap;    // true to allow wrapping
  phead[index].offend  = false;     // true once went off end or wrapped
  phead[index].curpos  = 0;         // always starts at 0
  phead[index].maxlen  = maxlen;    // distance to the next head
  phead[index].prevlen = 0;         // no previous length yet

  #if DEBUG_OUTPUT
  DBGOUT((F("  After:")));
  for (int i = 0; i < maxheads; ++i)
  {
    if (phead[i].dowrap || !phead[i].offend)
         DBGOUT((F("  #%d: DoWrap=%d OffEnd=%d CurPos=%-3d MaxLen=%-3d%s"), i,
                phead[i].dowrap, phead[i].offend, phead[i].curpos, phead[i].maxlen,
                ((i == index) ? "<==" : "")));
    //else DBGOUT((F("  #%d: unused"), i));
  }
  #endif

  return pData->inuse;
}

// draws all valid comet heads, returns number of heads currently in use
int PixelNutComets::cometHeadDraw(PixelNutComets::cometData cdata,
                                  PixelNutSupport::DrawProps *pdraw,
                                  PixelNutHandle handle, int pixlen)
{
  if (cdata == NULL) return 0;

  CometHeadData *pData = (CometHeadData*)cdata;
  CometHead *phead = pData->heads;

  for (int headnum = 0; headnum < pData->count; ++headnum, ++phead)
  {
    if (!phead->dowrap && phead->offend) // don't draw this head
      continue;

    #if 0 //DEBUG_OUTPUT
    int startbodylen = pdraw->pixCount;
    #endif
  
    int bodylen = pdraw->pixCount;
    if (bodylen == 1) bodylen = 2; // minimum body length (to clear previous body)
    int fadelen = bodylen-1;       // fade down tail with last pixel dark
  
    if (bodylen > phead->maxlen)   // if longer than length to following head
    {
        bodylen = phead->maxlen;   // shorten to avoid overwriting it
        fadelen = bodylen;         // fade into that following head
    }
    else
    if (bodylen < phead->prevlen)  // body has been shorted since last time
        bodylen = phead->prevlen;  // lengthen to avoid leaving a trail
                                   // but keep fadelen so erases that

    phead->prevlen = pdraw->pixCount; // save current count for next time

    int headpos = phead->curpos;   // current position of the head

    if (!phead->offend)
    {
      // adjust for new head just starting out so don't wrap around
      // (but don't adjust fade length so body grows naturally)
      if (bodylen > (headpos + 1))
          bodylen = (headpos + 1); // grow body each time
    }
  
    // establish max brightness and fade to black along tail
    float fade_scale = ((float)pdraw->pcentBright / MAX_PERCENTAGE);
    float fade_step = (fade_scale / fadelen);
  
    int curpos = headpos;
    int drawlen = bodylen; // drawing entire body, unless...

    if (headpos >= pixlen) // fallen off end
    {
      // adjust for pixels already off end
      int adjustpos = (headpos - pixlen);

      drawlen -= adjustpos;
      if (drawlen > 0)
      {
        // starting in middle of the fade
        fade_scale -= (adjustpos * fade_step);
        if (fade_scale < 0) fade_scale = 0;
      }
  
      curpos = pixlen-1; // start at ending pixel
    }
  
    #if 0 //DEBUG_OUTPUT
    DBGOUT((F("%2d: %sHeadPos=%-3d CurPos=%-3d StartBody=%-3d CurBody=%-3d DrawLen=%-3d FadeLen=%d"),
              headnum, (phead->offend ? " " : "^"), headpos, curpos, startbodylen, bodylen, drawlen, fadelen));
    //DBGOUT((F("    Fade(Scale=%-3d%% Step=%-3d%% Len=%d)"), (int)(fade_scale*100), (int)(fade_step*100), fadelen));
    #endif
  
    if (drawlen > 0)
    {
      while(1)
      {
        //DBGOUT((F("  %3d: DrawLen=%3d Scale=%3d%%"), curpos, drawlen, (int)(fade_scale*100)));
  
        pixelNutSupport.setPixel(handle, curpos, pdraw->r, pdraw->g, pdraw->b, fade_scale);
  
        if (!--drawlen) break;

        if (--curpos < 0) curpos = pixlen-1;
  
        fade_scale -= fade_step;
        if (fade_scale < 0) fade_scale = 0;
      }

      phead->curpos = ++headpos;
    }
    // else nothing to draw

    if (phead->dowrap) // if are wrapping check if at the end now
    {
      if (headpos >= pixlen)
      {
        phead->curpos = 0;
        phead->offend = true;
      }
    }
    else // if not wrapping check if body is completely done
    {
      if (headpos >= (pixlen + bodylen))
      {
        --pData->inuse;
        phead->offend = true;
        DBGOUT((F("Done: H%d(%d more) headpos=%d bodylen=%d"), headnum, pData->inuse, headpos, bodylen));
      }
    }
  }

  return pData->inuse;
}
