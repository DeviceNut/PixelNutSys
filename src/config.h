// Set project specific configuration defines here.
/*
Copyright (c) 2024, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#pragma once

#define PIXELNUT_VERSION        26          // 2.0+ - this is a rework of the 1st version

#define EEPROM_FORMAT           0           // 1 to clear entire flash data space
// This shouldn't normaly be necessary, as it is done automatically the very first time

// minimize these to reduce memory consumption:
#define MAXLEN_PATNAME          32          // max length for name of pattern
#define MAXLEN_PATSTR           1000        // must be long enough for all patterns
#define NUM_PLUGIN_TRACKS       16          // must be enough for largest pattern
#define NUM_PLUGIN_LAYERS       128         // must be multiple of TRACKS
#define DEV_PLUGINS             1           // we can support additional device plugins
#define PLUGIN_PLASMA           1           // uses Lissajious curves for effect

#if !defined(DEBUG_OUTPUT)  // can also be defined in each source file
#define DEBUG_OUTPUT 1      // 1 to compile serial console debugging code
#endif
#if DEBUG_OUTPUT
#undef F
#define F(x) x
#define DBG(x) x
#define DBGOUT(x) MsgFormat x
#define DEBUG_SIGNON            "PixelNut!"
#define SERIAL_BAUD_RATE        115200      // *must* match this in serial monitor
#define MSECS_WAIT_SERIAL       2000        // msecs to wait for serial monitor, or 0
#else
#define DBG(x)
#define DBGOUT(x)
#endif
extern void MsgFormat(const char *fmtstr, ...);

#include "mydevices.h" // put device specific settings here

// template for your own settings:
/*
      // these depend on what hardware is used and how it is wired:
      #define STRAND_COUNT            1           // physically separate strands
      #define PIXEL_COUNTS            { 16 }      // pixel counts for each strand
      #define PIXEL_PINS              { 21 }      // pin selects for each strand
      #define DPIN_LED                13          // on-board R-LED for error status

      #define APIN_MICROPHONE         0           // pin with microphone attached
      #define APIN_SEED               A0          // default pin for seeding randomizer
      #define PIXELS_APA              0           // define default value
      #define SPI_SETTINGS_FREQ       4000000     // use fastest speed by default

      // uncomment according to hardware controls used:
      //#define DPIN_PATTERN_BUTTON     0         // device pattern selection
      //#define DPIN_BRIGHT_BUTTON      0         // cycles global max brightness
      //#define APIN_BRIGHT_POT         0         // sets global max brightness
      //#define DPIN_DELAY_BUTTON       0         // cycles global delay offset
      //#define APIN_DELAY_POT          0         // sets global delay offset
      //#define DPIN_COUNT_BUTTON       0         // cycles pixel count property
      //#define APIN_COUNT_POT          0         // sets pixel count property
      //#define DPIN_TRIGGER_BUTTON     0         // initiates global trigger
      //#define APIN_TRIGGER_POT        0         // sets trigger force
      //#define DPIN_EMODE_BUTTON       0         // enables extern property mode
      //#define APIN_HUE_POT            0         // sets hue property
      //#define APIN_WHITE_POT          0         // sets white property

      // must have device patterns if not client app, or can have both
      //#define CLIENT_APP              1           // have external application
      //#define DEV_PATTERNS            1           // 1 to add device patterns

      // for use with external client: only one can be set:
      //#define BLE_ESP32               1           // BLE on ESP32 only
      //#define WIFI_MQTT               1           // MQTT over WiFi
      //#define WIFI_SOFTAP             1           // SoftAP over WiFi
      //#define COM_SERIAL              1           // serial over COM
*/

// these are defaults for particular global settings:

#if !defined(MAX_BRIGHTNESS)
#define MAX_BRIGHTNESS          100         // default is to allow for maximum brightness
#endif

#if !defined(PIXEL_OFFSET)
#define PIXEL_OFFSET            0           // start drawing at the first pixel
#endif

#if !defined(DEV_PATTERNS)
#define DEV_PATTERNS            1           // use internal device patterns
#endif

#if !defined(MSECS_WAIT_WIFI)
#define MSECS_WAIT_WIFI         5000        // msecs to wait for original WiFi connection
#endif

#if !defined(APIN_SEED) && !defined(ESP32)  // ESP32 has random number generator
#define APIN_SEED               A0          // default pin for seeding randomizer
#endif

#if !defined(PIXELS_APA)
#define PIXELS_APA              0
#elif !defined(SPI_SETTINGS_FREQ)
#define SPI_SETTINGS_FREQ       4000000     // use fastest speed by default
#endif

#if defined(__arm__) && defined(__MK20DX256__)
#define TEENSY_32               1
#endif

#if defined(__arm__) && defined(APIN_MICROPHONE)
#define PLUGIN_SPECTRA          1           // uses audio input (only works on ARM processors)
#define FREQ_FFT                1           // FFT applied to audio input
#else
#define PLUGIN_SPECTRA          0
#endif

// surpress warnings for undefined symbols
#if !defined(BLE_ESP32)
#define BLE_ESP32               0
#endif
#if !defined(WIFI_SOFTAP)
#define WIFI_SOFTAP             0
#endif
#if !defined(COM_SERIAL)
#define COM_SERIAL              0
#endif

#if (BLE_ESP32 || WIFI_MQTT || WIFI_SOFTAP || COM_SERIAL)
#define DEFAULT_DEVICE_NAME     "PixelNutDevice" // default name of device
#define MAXLEN_DEVICE_NAME      32          // maxlen for device name
#define PREFIX_DEVICE_NAME      "P!!"       // for name to be recognized
#define CLIENT_APP              1           // have external application
#else
#define CLIENT_APP              0           // no external application
#endif
