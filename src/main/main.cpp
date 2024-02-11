/*
Copyright (c) 2024, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"
#include "main/flash.h"
#include "controls/controls.h"

#include <NeoPixelShow.h>

#define MACRO_TO_STR(s) #s
#define MACRO_TO_MACRO(s) MACRO_TO_STR(s)
#pragma message("Building: " MACRO_TO_MACRO(BUILD_DEVICE))

extern void SetupLED(void);
extern void SetupDBG(void);
extern void LoadCurPattern();

bool doUpdate = true;             // false to not update display

#if DEV_PATTERNS
byte codePatterns = 0;            // number of internal patterns
byte curPattern = 1;              // current pattern (1..codePatterns)
#endif

#if !CLIENT_APP
CustomCode customcode;            // create empty base class
CustomCode *pCustomCode = &customcode;
#endif

#if PIXELS_APA
#include <SPI.h>
SPISettings spiSettings(SPI_SETTINGS_FREQ, MSBFIRST, SPI_MODE0);
PixelValOrder pixorder = {2,1,0}; // mapping of (RGB) to (BRG) for APA102
#else
PixelValOrder pixorder = {1,0,2}; // mapping of (RGB) to (GRB) for WS2812B
NeoPixelShow *neoPixels[STRAND_COUNT];
#endif
PixelNutSupport pixelNutSupport = PixelNutSupport((GetMsecsTime)millis, &pixorder);

PixelNutEngine pixelNutEngines[STRAND_COUNT];
PixelNutEngine *pPixelNutEngine; // pointer to current engine

static int pixcounts[] = PIXEL_COUNTS;
static byte pinnums[] = PIXEL_PINS;
#define PIXEL_BYTES 3 // fixed for all pixels

#if DEBUG_OUTPUT
#warning("Debug mode is enabled")
#endif

void DisplayConfiguration(void)
{
  #if DEBUG_OUTPUT

  char numstr[20];
  char pixstr[100];
  char pinstr[100];
  pixstr[0] = pinstr[0] = 0;
  for (int i = 0; i < STRAND_COUNT; ++i)
  {
    itoa(pixcounts[i], numstr, 10);
    strcat(pixstr, numstr);
    strcat(pixstr, " ");

    itoa(pinnums[i], numstr, 10);
    strcat(pinstr, numstr);
    strcat(pinstr, " ");
  }

  DBGOUT((F("Configuration:")));
  DBGOUT((F("  STRAND_COUNT         = %d"), STRAND_COUNT));
  DBGOUT((F("  PIXEL_COUNTS         = %s"), pixstr));
  DBGOUT((F("  PIXEL_PINS           = %s"), pinstr));
  DBGOUT((F("  MAXLEN_PATSTR        = %d"), MAXLEN_PATSTR));
  DBGOUT((F("  MAXLEN_PATNAME       = %d"), MAXLEN_PATNAME));
  DBGOUT((F("  DEV_PATTERNS         = %d"), DEV_PATTERNS));
  DBGOUT((F("  CLIENT_APP           = %d"), CLIENT_APP));
  DBGOUT((F("  NUM_PLUGIN_TRACKS    = %d"), NUM_PLUGIN_TRACKS));
  DBGOUT((F("  NUM_PLUGIN_LAYERS    = %d"), NUM_PLUGIN_LAYERS));
  DBGOUT((F("  FLASHOFF_PINFO_START = %d"), FLASHOFF_PINFO_START));
  DBGOUT((F("  FLASHOFF_PINFO_END   = %d"), FLASHOFF_PINFO_END));
  DBGOUT((F("  EEPROM_FREE_BYTES    = %d"), EEPROM_FREE_BYTES));

  #if defined(ESP32)
  esp_chip_info_t sysinfo;
  esp_chip_info(&sysinfo);
  DBGOUT(("ESP32 Board:"));
  DBGOUT(("  SDK Version=%s", esp_get_idf_version()));
  DBGOUT(("  ModelRev=%d.%d", sysinfo.model, sysinfo.revision));
  DBGOUT(("  Cores=%d", sysinfo.cores));
  DBGOUT(("  Heap=%d bytes", esp_get_free_heap_size()));
  #endif

  #endif
}

// int count = 3;
// uint32_t *rmtptr = 0;

void ShowPixels(int index)
{
  int pcount = pixelNutEngines[index].numPixels;
  byte *ppix = pixelNutEngines[index].pDrawPixels;

  #if PIXELS_APA

  digitalWrite(pinnums[index], LOW); // enable this strand
  SPI.beginTransaction(spiSettings);

  // 4 byte start-frame marker
  for (int i = 0; i < 4; i++) SPI.transfer(0x00);

  for (int i = 0; i < pcount; ++i)
  {
    SPI.transfer(0xFF);
    for (int j = 0; j < PIXEL_BYTES; j++) SPI.transfer(*ppix++);
  }

  SPI.endTransaction();
  digitalWrite(pinnums[index], HIGH); // disable this strand

  #else

  // --count;
  // if (count >= 0)
  // {
  //   DBGOUT((F("ShowPixels:")));
  //   byte *p = ppix;
  //   for (int i = 0; i < pcount; ++i)
  //     DBGOUT((F("%02x.%02x.%02x"), *p++, *p++, *p++));
  //   DBGOUT((F("--------")));
  // }
  // else return;

  neoPixels[index]->show(ppix, pcount*PIXEL_BYTES);

  // if (count >= 0)
  // {
  //   DBGOUT((F("rmtptr=%08X"), rmtptr));
  //   uint32_t *p2 = rmtptr;
  //   for (int i = 0; i < pcount; ++i)
  //   {
  //     for (int i = 0; i < PIXEL_BYTES; ++i)
  //       DBGOUT((F("%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x"), *p2++, *p2++, *p2++, *p2++, *p2++, *p2++, *p2++, *p2++));

  //     DBGOUT((F("---")));
  //   }
  //   DBGOUT((F("------------------")));
  // }
  #endif
}

void setup()
{
  SetupLED(); // status LED: indicate in setup now
  SetupDBG(); // setup/wait for debug monitor
  // Note: cannot use debug output until above is called,
  // meaning DBGOUT() cannot be used in static constructors.

  #if EEPROM_FORMAT
  FlashFormat(); // format entire EEPROM
  ErrorHandler(0, 3, true);
  #endif

  #if PIXELS_APA
  for (int i = 0; i < STRAND_COUNT; ++i) // config chip select pins
  {
    pinMode(pinnums[i], OUTPUT);
    digitalWrite(pinnums[i], HIGH);
  }
  SPI.begin(); // initialize SPI library
  #endif

  DisplayConfiguration(); // Display configuration settings

  #if DEV_PATTERNS
  CountPatterns(); // have internal stored patterns
  #endif

  // alloc arrays, turn off pixels, init patterns
  for (int i = 0; i < STRAND_COUNT; ++i)
  {
    #if !PIXELS_APA
    // DBGOUT((F("Alloc neopixelshow...")));
    neoPixels[i] = new NeoPixelShow(pinnums[i]);
    if (neoPixels[i] == NULL)
    {
      DBGOUT((F("Alloc failed for neopixel class, strand=%d"), i));
      ErrorHandler(1, 0, true);
    }
    #endif

    #if defined(ESP32)
    // crashes if pin isn't set correctly
    // DBGOUT((F("Alloc RMT buffer...")));
    if (!neoPixels[i]->rmtInit(i, (pixcounts[i] * PIXEL_BYTES))) //, &rmtptr))
    {
      DBGOUT((F("Alloc failed for RMT data, strand=%d"), i));
      ErrorHandler(1, 0, true);
    }
    #endif

    // DBGOUT((F("Alloc pixelnut engines...")));
    if (!pixelNutEngines[i].init(pixcounts[i], PIXEL_BYTES, NUM_PLUGIN_LAYERS, NUM_PLUGIN_TRACKS, PIXEL_OFFSET))
    {
      DBGOUT((F("Failed to initialize pixel engine, strand=%d"), i));
      ErrorHandler(2, PixelNutEngine::Status_Error_Memory, true);
    }

    pPixelNutEngine = &pixelNutEngines[i];
    ShowPixels(i); // turn off pixels

    FlashSetStrand(i);
    FlashStartup(); // get curPattern and settings from flash, set engine properties

    // sets engine properties, so must do for each strand
    SetupBrightControls();  // Setup any physical controls present
    SetupDelayControls();
    SetupEModeControls();
    SetupColorControls();
    SetupCountControls();
    SetupTriggerControls();
    SetupPatternControls();

    #if CLIENT_APP
    char cmdstr[MAXLEN_PATSTR+1];
    FlashGetPatStr(cmdstr); // get pattern string previously stored in flash
    ExecPattern(cmdstr);    // load pattern into the engine: ready to be displayed
    #else
    LoadCurPattern();       // load pattern string corresponding to pattern number
    #endif
  }

  if (STRAND_COUNT > 1)
  {
    FlashSetStrand(0); // always start on first strand
    pPixelNutEngine = &pixelNutEngines[0];
  }

  pCustomCode->setup(); // custom initialization here

  #if defined(ESP32)
  randomSeed(esp_random()); // should be called after BLE/WiFi started
  #else
  // set seed to value read from unconnected analog port
  randomSeed(analogRead(APIN_SEED));
  #endif

  pCustomCode->sendReply((char*)"<Reboot>"); // signal client

  BlinkStatusLED(0, 2); // indicates success
  DBGOUT((F("** Setup complete **")));
}

void loop()
{
  pCustomCode->loop(); // custom processing here

  // check physical controls for changes
  CheckBrightControls();
  CheckDelayControls();
  CheckEModeControls();
  CheckColorControls();
  CheckCountControls();
  CheckTriggerControls();
  CheckPatternControls();

  // if enabled: display new pixel values if anything has changed
  if (doUpdate)
    for (int i = 0; i < STRAND_COUNT; ++i)
      if (pixelNutEngines[i].updateEffects())
        ShowPixels(i);
}
