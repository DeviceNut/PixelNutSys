// Set project specific configuration defines here.
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#pragma once

#if !defined(DEBUG_OUTPUT)  // can also be defined in each source file
#define DEBUG_OUTPUT 1      // 1 to compile serial console debugging code
#endif

#if DEBUG_OUTPUT
#define DBG(x) x
#define DBGOUT(x) MsgFormat x
#define DEBUG_SIGNON            "PixelNut!"
#define SERIAL_BAUD_RATE        115200      // *must* match this in serial monitor
#define MSECS_WAIT_FOR_USER     0 //15000       // msecs to wait for serial monitor, or 0
#else
#define DBG(x)
#define DBGOUT(x)
#endif
#undef F
#define F(x) x
extern void MsgFormat(const char *fmtstr, ...);

#define PIXELNUT_VERSION        20          // 2.0 - this is a rework of the 1st version

// to initilize the flash, set this to 1 to clear the EEPROM; must be 0 for normal operation
#define EEPROM_FORMAT           0           // 1 to clear entire flash data space

// these are defaults for particular global settings:
#define MAX_BRIGHTNESS          100         // default is to allow for maximum brightness
#define PIXEL_OFFSET            0           // start drawing at the first pixel

#if defined(__arm__) && defined(__MKL26Z64__)
#define TEENSY_LC               1
#endif
#if defined(__arm__) && defined(__MK20DX256__)
#define TEENSY_32               1
#endif

// small RAM if AVR or TeensyLC
#if defined(__AVR__) || TEENSY_LC
#define LARGE_RAM               0           // has only 2k of RAM
#else
#define LARGE_RAM               1           // 1 for processors with larger RAM
#endif

// minimize these to reduce memory consumption:
#if LARGE_RAM
#define MAXLEN_PATSTR           1024        // must be long enough for patterns
#define MAXLEN_PATNAME          32          // max length for name of pattern
#define NUM_PLUGIN_TRACKS       16          // must be enough for patterns
#define NUM_PLUGIN_LAYERS       64          // must be multiple of TRACKS
#define DEV_PLUGINS             1           // cannot support additional plugins
#define PLUGIN_PLASMA           1           // uses Lissajious curves for effect
#define PLUGIN_SPECTRA          0           // uses audio input (must set APIN_MICROPHONE and FREQ_FFT)
                                            // ** only works on ARM processors right now
#else
#define MAXLEN_PATSTR           300         // must be long enough for patterns
#define MAXLEN_PATNAME          32          // max length for name of pattern
#define NUM_PLUGIN_TRACKS       4           // must be enough for patterns
#define NUM_PLUGIN_LAYERS       16          // must be multiple of TRACKS
#define DEV_PLUGINS             0           // 1 to add device plugins
#endif

// these depend on what hardware is used and how it is wired:
#define STRAND_COUNT            1           // physically separate strands
#define PIXEL_BYTES             3           // number of bytes per pixel
#define PIXEL_COUNTS            { 16 }      // pixel counts for each strand
#define PIXEL_PINS              { 21 }      // pin selects for each strand
#define DPIN_LED                13          // on-board R-LED for error status (Adafruit Feather)
#define APIN_SEED               A0          // default pin for seeding randomizer
#define PIXELS_APA              0           // define default value
#define SPI_SETTINGS_FREQ       4000000     // use fastest speed by default

#if PLUGIN_SPECTRA
#define APIN_MICROPHONE         0           // pin with microphone attached
#define FREQ_FFT                0           // FFT applied to audio input
#endif

// uncomment according to hardware controls used:
/*
#define DPIN_PATTERN_BUTTON     0         // device pattern selection (only if !CLIENT_APP)
#define DPIN_BRIGHT_BUTTON      0         // cycles global max brightness
#define APIN_BRIGHT_POT         0         // sets global max brightness
#define DPIN_DELAY_BUTTON       0         // cycles global delay offset
#define APIN_DELAY_POT          0         // sets global delay offset
#define DPIN_COUNT_BUTTON       0         // cycles pixel count property
#define APIN_COUNT_POT          0         // sets pixel count property
#define DPIN_TRIGGER_BUTTON     0         // initiates global trigger
#define APIN_TRIGGER_POT        0         // sets trigger force
#define DPIN_EMODE_BUTTON       0         // enables extern property mode
#define APIN_HUE_POT            0         // sets hue property
#define APIN_WHITE_POT          0         // sets white property
*/

#if defined(__AVR__)
#define CLIENT_APP              0           // only hardware controls
#define DEV_PATTERNS            1           // must have internal patterns
#else
// for use with external client: only one can be set:
#define CLIENT_APP              1           // have external application
#define DEV_PATTERNS            1           // 1 to add device patterns

#if TEENSY_LC || TEENSY_32
#define COM_SERIAL              1           // serial over COM
#else
#define BLE_ESP32               0           // BLE on ESP32 only
#define WIFI_MQTT               1           // MQTT over WiFi
#define WIFI_SOFTAP             0           // SoftAP over WiFi
#define COM_SERIAL              0           // serial over COM
#endif

#if (BLE_ESP32 || WIFI_MQTT || WIFI_SOFTAP || COM_SERIAL)
#define DEFAULT_DEVICE_NAME     "PixelNutDevice" // name of the device
#define MAXLEN_DEVICE_NAME      16          // maxlen for device name
#define PREFIX_DEVICE_NAME      "P!"        // for name to be recognized
#endif
#endif
