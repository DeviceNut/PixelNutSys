/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"
#include "main/flash.h"
#if !defined(PARTICLE)
#include <EEPROM.h>
#endif

#if (FLASHOFF_PINFO_END > EEPROM_BYTES)
#error("Not enough flash space to store external pattern strings");
#endif

static byte valOffset = FLASHOFF_STRAND_DATA;

#if CLIENT_APP
static uint16_t pinfoOffset = FLASHOFF_PINFO_START;
#endif

static void FlashStart(void)
{
#if defined(ESP32)
  EEPROM.begin(EEPROM_BYTES);
#endif
}

static void FlashDone(void)
{
#if defined(ESP32)
  EEPROM.commit();
#endif
}

void FlashSetValue(uint16_t offset, byte value) { EEPROM.write(valOffset + offset, value); }
byte FlashGetValue(uint16_t offset) { return EEPROM.read(valOffset + offset); }

byte FlashGetStrand(void) { return (valOffset - FLASHOFF_STRAND_DATA) / FLASHLEN_STRAND_DATA; }

// Note: this is not range checked
void FlashSetStrand(byte strandindex)
{
  valOffset = FLASHOFF_STRAND_DATA + (strandindex * FLASHLEN_STRAND_DATA);
  #if CLIENT_APP
  pinfoOffset = (FLASHOFF_PINFO_START + (strandindex * (FLASHLEN_PATNAME + FLASHLEN_PATSTR)));
  #endif
}

#if CLIENT_APP

void FlashSetDevName(char *name)
{
  DBGOUT((F("FlashSetDevName: \"%s\""), name));

  for (int i = 0; i < MAXLEN_DEVICE_NAME; ++i)
  {
    EEPROM.write(i, name[i]);
    if (!name[i]) break;
  }

  FlashDone();
}

void FlashGetDevName(char *name)
{
  for (int i = 0; i < MAXLEN_DEVICE_NAME; ++i)
  {
    name[i] = EEPROM.read(i);
    if(!name[i]) break;
  }

  name[MAXLEN_DEVICE_NAME] = 0; // insure termination

  DBGOUT((F("FlashGetDevName: \"%s\""), name));
}

void FlashSetPatName(char *name)
{
  DBGOUT((F("FlashSetPatName(@%d): \"%s\"  (len=%d)"),
          pinfoOffset, name, strlen(name)));

  for (int i = 0; i < MAXLEN_PATNAME; ++i)
  {
    EEPROM.write((pinfoOffset + i), name[i]);
    if (!name[i]) break;
  }

  FlashDone();
}

void FlashGetPatName(char *name)
{
  for (int i = 0; i < MAXLEN_PATNAME; ++i)
  {
    name[i] = EEPROM.read(pinfoOffset + i);
    if (!name[i]) break;
  }
  name[MAXLEN_PATNAME] = 0; // insure termination

  DBGOUT((F("FlashGetPatName(@%d): \"%s\"  (len=%d)"),
          pinfoOffset, name, strlen(name)));
}

void FlashSetPatStr(char *str)
{
  DBGOUT((F("FlashSetPatStr(@%d): \"%s\" (len=%d)"),
          (pinfoOffset + FLASHLEN_PATNAME), str, strlen(str)));

  for (int i = 0; i < MAXLEN_PATSTR; ++i)
  {
    EEPROM.write((pinfoOffset + FLASHLEN_PATNAME + i), str[i]);
    if (!str[i]) break;
  }

  FlashDone();
}

void FlashGetPatStr(char *str)
{
  for (int i = 0; i < MAXLEN_PATSTR; ++i)
  {
    str[i] = EEPROM.read(pinfoOffset + FLASHLEN_PATNAME + i);
    if (!str[i]) break;
  }
  str[MAXLEN_PATSTR] = 0; // insure termination

  DBGOUT((F("FlashGetPatStr(@%d): \"%s\" (len=%d)"),
          (pinfoOffset + FLASHLEN_PATNAME), str, strlen(str)));
}

#endif // CLIENT_APP

void FlashSetPatNum(byte pattern) { FlashSetValue(FLASHOFF_SDATA_PATNUM, pattern); FlashDone(); }

void FlashSetBright() { FlashSetValue(FLASHOFF_SDATA_PC_BRIGHT, pPixelNutEngine->getBrightPercent());  FlashDone(); }
void FlashSetDelay()  { FlashSetValue(FLASHOFF_SDATA_PC_DELAY,  pPixelNutEngine->getDelayPercent());   FlashDone(); }
void FlashSetFirst()  { FlashSetValue(FLASHOFF_SDATA_FIRSTPOS,  pPixelNutEngine->getFirstPosition());  FlashDone(); }

void FlashSetXmode(bool enable) { FlashSetValue(FLASHOFF_SDATA_XT_MODE, enable);  FlashDone(); }

void FlashSetExterns(byte hue, byte wht, byte cnt)
{
  FlashSetValue(FLASHOFF_SDATA_XT_HUE, hue);
  FlashSetValue(FLASHOFF_SDATA_XT_WHT, wht);
  FlashSetValue(FLASHOFF_SDATA_XT_CNT, cnt);
  FlashDone();
}

void FlashStartup(void)
{
  FlashStart();

  #if DEV_PATTERNS
  curPattern = FlashGetValue(FLASHOFF_SDATA_PATNUM);
  if (!curPattern || (curPattern > codePatterns))
    curPattern = 1; // 1..codePatterns
  DBGOUT((F("Flash: pattern=#%d"), curPattern));
  #endif

  byte bright = FlashGetValue(FLASHOFF_SDATA_PC_BRIGHT);
  if (!bright || (bright > MAX_BRIGHTNESS))
  {
    DBGOUT((F("Resetting bright: %d => %d %%"), bright, MAX_BRIGHTNESS));
    FlashSetValue(FLASHOFF_SDATA_PC_BRIGHT, bright=MAX_BRIGHTNESS);
  }

  byte delay = FlashGetValue(FLASHOFF_SDATA_PC_DELAY);
  if (delay > MAX_PERCENTAGE)
  {
    DBGOUT((F("Resetting delay: %d => %d %%"), delay, MAX_PERCENTAGE));
    FlashSetValue(FLASHOFF_SDATA_PC_DELAY, delay=MAX_PERCENTAGE);
  }

  int16_t fpos = FlashGetValue(FLASHOFF_SDATA_FIRSTPOS);

  DBGOUT((F("Flash: bright=%d%% delay=%d%%"), bright, delay));

  pPixelNutEngine->setBrightPercent(bright);
  pPixelNutEngine->setDelayPercent(delay);
  pPixelNutEngine->setFirstPosition(fpos);

  pPixelNutEngine->setPropertyMode(FlashGetValue(FLASHOFF_SDATA_XT_MODE));
  pPixelNutEngine->setColorProperty(FlashGetValue(FLASHOFF_SDATA_XT_HUE),
                                    FlashGetValue(FLASHOFF_SDATA_XT_WHT));
  pPixelNutEngine->setCountProperty(FlashGetValue(FLASHOFF_SDATA_XT_CNT));

  FlashDone();
}

#if EEPROM_FORMAT
void FlashFormat(void)
{
  FlashStart();
  for (int i = 0; i < EEPROM_BYTES; ++i) EEPROM.write(i, 0);
  FlashDone();

  DBGOUT((F("Cleared %d bytes of EEPROM"), EEPROM_BYTES));
}
#endif
