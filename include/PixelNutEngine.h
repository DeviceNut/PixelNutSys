/*
    Copyright (c) 2021, Greg de Valois
    Software License Agreement (MIT License)
    See license.txt for the terms of this license.
*/

#pragma once

class PixelNutEngine
{
public:

  enum Status // Returned value from 'execCmdStr()' call below
  {
    Status_Success=0,
    Status_Error_BadVal,        // invalid value used in command
    Status_Error_BadCmd,        // unrecognized/invalid command
    Status_Error_Memory,        // not enough memory for effect
  };

  // These bits are combined to create the value for the 'Q' command for an individual track
  // (drawing effect). It is used to direct external control of the color/count properties
  // to particular tracks, such that changing those controls only affects tracks that have
  // the corresponding bit set.
  //
  // In addition, when the external property mode is enabled with 'setPropertyMode()',
  // a set property bit prevents any predraw effect from changing that property for that track.
  // Otherwise, a set bit allows modification by both external and internal (predraw) sources.
  //
  // By default (no bits set), only predraw effects can change the drawing properties.
  enum ExtControlBit
  {                                 // corresponds to the drawing property:
    ExtControlBit_DegreeHue  = 1,   //  degreeHue
    ExtControlBit_PcentWhite = 2,   //  pcentWhite
    ExtControlBit_PixCount   = 4,   //  pixCount
    ExtControlBit_All        = 7    // all bits ORed together
  };

  // initializer: set number and length of the pixels to be drawn, 
  // the first pixel to start drawing and the direction of drawing,
  // and the maximum effect layers and tracks that can be supported.
  // returns false if failed (not enough memory)
  bool init(uint16_t num_pixels, byte pixel_bytes,
            byte num_layers, byte num_tracks,
            uint16_t first_pixel=0, bool backwards=false);

  void setBrightPercent(byte percent) { pcentBright = percent; }
  byte getBrightPercent() { return pcentBright; }

  void setDelayPercent(byte percent) { pcentDelay = percent; }
  byte getDelayPercent() { return pcentDelay; }

  void setFirstPosition(uint16_t pixpos)
  {
    if (pixpos < 0) pixpos = 0;
    if (numPixels <= pixpos) pixpos = numPixels-1;
    firstPixel = pixpos;
  }
  uint16_t getFirstPosition() { return firstPixel; }

  // Sets the color properties for tracks that have set either the ExtControlBit_DegreeHue
  // or ExtControlBit_PcentWhite bits. These values can be individually controlled. The
  // 'hue_degree' is a value from 0...MAX_DEGREES_CIRCLE, and the 'white_percent' value
  // is a percentage from 0...MAX_PERCENTANGE.
  void setColorProperty(short hue_degree, byte white_percent);

  // Sets the pixel count property for tracks that have set the ExtControlBit_PixCount bit.
  // The 'pixcount_percent' value is a percentage from 0...MAX_PERCENTAGE.
  void setCountProperty(byte pixcount_percent);

  // When enabled, predraw effects are prevented from modifying the color/count properties
  // of a track with the corresponding ExtControlBit bit set, allowing only the external
  // control of that property with calls to set..Property().
  void setPropertyMode(bool enable);

  // Retrieves the external property mode settings
  bool  getPropertyMode()    { return externPropMode;   }
  short getPropertyHue()     { return externDegreeHue;  }
  byte  getPropertyWhite()   { return externPcentWhite; }
  byte  getPropertyCount()   { return externPcentCount; }

  // Triggers effect layers with a range value of -MAX_FORCE_VALUE..MAX_FORCE_VALUE.
  // (Negative values are not utilized by most plugins: they take the absolute value.)
  // Must be enabled with the "I" command for each effect layer to be effected.
  void triggerForce(short force);

  // Used by plugins to trigger based on the effect layer, enabled by the "A" command.
  void triggerForce(byte layer, short force);

  // Called by the above and internal triggering functions to allow override
  virtual void triggerLayer(byte layer, short force);

  // Parses and executes a pattern command string, returning a status code.
  // An empty string (or one with only spaces), is ignored.
  virtual Status execCmdStr(char *cmdstr);

  virtual void clearStacks(void); // Pops off all layers from the stack

  // Updates current effect: returns true if the pixels have changed and should be redisplayed.
  virtual bool updateEffects(void);

  // creates command string from currently running pattern
  bool makeCmdStr(char *cmdstr, int maxlen);

  // Used to access main display buffer and related parameters.
  byte *pDrawPixels;    // current pixel buffer to draw into or display
  uint16_t numPixels;   // number of pixels in output buffer
  uint16_t pixelBytes;  // total bytes in for all pixels

protected:

  // default values for propertes and control settings:
  #define DEF_PCENTBRIGHT   MAX_PERCENTAGE
  #define DEF_PCENTDELAY    (MAX_PERCENTAGE/2)
  #define DEF_DEGREESHUE    275
  #define DEF_PCENTWHITE    0
  #define DEF_PCENTCOUNT    50
  #define DEF_BACKWARDS     false
  #define DEF_PIXORVALS     false
  #define DEF_FORCEVAL      (MAX_FORCE_VALUE/2)
  #define DEF_TRIG_FOREVER  0  // repeat forever
  #define DEF_TRIG_OFFSET   0
  #define DEF_TRIG_RANGE    0
  #define SETVAL_IF_NONZERO(var,val) {if (val != 0) var = val;}

  // saves what triggering is enabled
  enum TrigTypeBit
  {
    TrigTypeBit_AtStart      = 1,   // starting trigger ("T" command)
    TrigTypeBit_External     = 2,   // external source  ("I" command)
    TrigTypeBit_Internal     = 4,   // internal source  ("A" command)
    TrigTypeBit_Repeating    = 8,   // auto-repeating   ("R" command)
  };

  byte pcentBright = MAX_BRIGHTNESS;            // percent brightness to apply to each effect
  byte pcentDelay  = MAX_PERCENTAGE/2;          // percent delay to apply to each effect

  struct ATTR_PACKED _PluginTrack;
  typedef struct ATTR_PACKED // 30-34 bytes
  {
    struct _PluginTrack *pTrack;                // pointer to track for this layer
    PixelNutPlugin *pPlugin;                    // pointer to the created plugin object
    uint16_t iplugin;                           // plugin ID value
    bool redraw;                                // true if plugin is drawing else filter
    bool disable;                               // true to disable this layer (mute)
    byte reserved;

    byte trigType;                              // which triggers have been set (TrigTypeBit_xx)
    bool trigActive;                            // true once layer has been triggered once
    byte trigLayerIndex;                        // layer stack index of effect trigger
    uint16_t trigLayerID;                       //  and the layerID of that layer
    short trigForce;                            // amount of force to apply (-1 for random)

                                                // repeat triggering:
    uint16_t trigRepCount;                      // number of times to trigger (0 to repeat forever)
    uint16_t trigDnCounter;                     // current trigger countdown counter
    uint32_t trigTimeMsecs;                     // next trigger time in msecs, calculated from:
    uint16_t trigRepOffset;                     // min delay offset before next trigger in seconds
    uint16_t trigRepRange;                      // range of delay values possible (min...min+range)

    uint16_t thisLayerID;                       // unique identifier for this layer
  }
  PluginLayer; // defines each layer of effect plugin

  typedef struct ATTR_PACKED _PluginTrack // 28-32 bytes
  {
    PluginLayer *pLayer;                        // pointer to layer for this track
    byte *pBuffer;                              // pixel buffer for this track

    PixelNutSupport::DrawProps draw;            // drawing properties for this track
    uint32_t msTimeRedraw;                      // time of next redraw of plugin in msecs

    bool active;                                // true if has been activated (G command)
    byte ctrlBits;                              // controls setting properties (ExtControlBit_xx)
    byte lcount;                                // number of layers in this track (>= 1)
    byte reserved;
  }
  PluginTrack; // defines properties for each drawing plugin

  PluginLayer *pluginLayers;                    // plugin layers that creates effect
  short maxPluginLayers;                        // max number of layers possible
  short indexLayerStack = -1;                   // index into the plugin layers stack
  uint16_t uniqueLayerID = 1;                   // used to identify layers uniquely

  PluginTrack *pluginTracks;                    // plugin tracks that have properties
  short maxPluginTracks;                        // max number of tracks possible
  short indexTrackStack = -1;                   // index into the plugin properties stack
  short indexTrackEnable = -1;                  // higher indices are not yet activated

  uint32_t msTimeUpdate = 0;                    // time of previous call to update
  uint16_t maxDelayMsecs = 500;                 // delay time in msecs needed to get 1Hz
                                                // (needs to be calibrated on bootup) TODO

  uint16_t firstPixel = 0;                      // offset to the start of the drawing array
  bool goBackwards = false;                     // false to draw from start to end, else reverse

  byte numBytesPerPixel;                        // number of bytes needed for each pixel
  byte *pDisplayPixels;                         // pointer to actual output display pixels

  bool externPropMode = false;                  // true to allow external control of properties
  short externDegreeHue;                        // externally set values property values
  byte externPcentWhite;
  byte externPcentCount;

  void SetPropColor(void);
  void SetPropCount(void);
  void RestorePropVals(PluginTrack *pTrack, uint16_t pixCount, uint16_t degreeHue, byte pcentWhite);

  void RepeatTriger(bool rollover);

  Status MakeNewPlugin(uint16_t iplugin, PixelNutPlugin **ppPlugin);
  void InitPluginTrack(PluginTrack *pTrack, PluginLayer *pLayer);
  void InitPluginLayer(PluginLayer *pLayer, PluginTrack *pTrack,
                          PixelNutPlugin *pPlugin, uint16_t iplugin, bool redraw);

  void updateTrackPtrs(void);
  void updateLayerPtrs(void);

  Status AppendPluginLayer(uint16_t plugin);
  Status InsertPluginLayer(short layer, uint16_t plugin);
  Status SwitchPluginLayer(short layer, uint16_t plugin);
  Status SwapPluginLayers(short layer);
  void DeletePluginLayer(short layer);
};

class PluginFactory
{
  public: virtual byte *pluginList(void);
  public: virtual bool pluginBits(uint16_t plugin);
  public: virtual bool pluginDraws(uint16_t plugin);
  public: virtual PixelNutPlugin *pluginCreate(uint16_t plugin);
};
