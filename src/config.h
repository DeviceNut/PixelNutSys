// Set project specific configuration defines here.
/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#pragma once

#if !defined(DEBUG_OUTPUT)
#define DEBUG_OUTPUT 1 // 1 to compile serial console debugging code
#endif

#if DEBUG_OUTPUT // defined either in main.h or in particular source file
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

// when first flash a new device, set this to 1 to clear the EEPROM, then reset to 0
#define EEPROM_FORMAT           0           // 1 to write 0 to entire flash data space

// these depend on power comsumption, processor speed, and the physical layout:
#define MAX_BRIGHTNESS          100         // default is to allow for maximum brightness
#define DELAY_OFFSET            0           // default is no additional delay
#define DELAY_RANGE             60          // +/- allowed range of msecs delay
#define PIXEL_OFFSET            0           // start drawing at the first pixel
#define DIRECTION_UP            true        // draw from start to end by default

// minimize these to reduce memory consumption:
#define STRLEN_PATTERNS         300         // must be long enough for patterns
#define NUM_PLUGIN_LAYERS       16          // must be multiple of TRACKS
#define NUM_PLUGIN_TRACKS       4           // must be enough for patterns

// for device plugins and patterns:
#define DEV_PATTERNS            1           // 1 to add device patterns
#define DEV_PLUGINS             0           // 1 to add device plugins
#define PLUGIN_PLASMA           1           // uses Lissajious curves for effect
#define PLUGIN_SPECTRA          0           // uses audio input (must set APIN_MICROPHONE and FREQ_FFT)
// NOTE: Spectra only works on ARM processors right now

// these depend on what hardware is used and how it is wired:
#define STRAND_COUNT            1           // physically separate strands
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

// for use with external client: only one can be set:
#define BLE_ESP32               0           // BLE on ESP32 only
#define WIFI_MQTT               1           // MQTT over WiFi
#define WIFI_SOFTAP             0           // SoftAP over WiFi
#define COM_SERIAL              0           // serial over COM

#if (BLE_ESP32 || WIFI_MQTT || WIFI_SOFTAP || COM_SERIAL)
#define DEFAULT_DEVICE_NAME     "PixelNutDevice" // name of the device
#define MAXLEN_DEVICE_NAME      16          // maxlen for device name
#define PREFIX_DEVICE_NAME      "P!"        // for name to be recognized
#define PREFIX_LEN_DEVNAME      2           // length of this prefix
#define CLIENT_APP              1           // have external application
#else
#define CLIENT_APP              0           // only hardware controls
#endif
