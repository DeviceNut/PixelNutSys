/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"
#include "flash.h"
#include <EEPROM.h>

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

// Note: this is not range checked
byte FlashSetStrand(byte strandindex)
{
  int oldval = (valOffset - FLASHOFF_STRAND_DATA) / FLASHLEN_STRAND_DATA;
  valOffset = FLASHOFF_STRAND_DATA + (strandindex * FLASHLEN_STRAND_DATA);
  return oldval;
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

void FlashSetPatStr(char *str)
{
  DBGOUT((F("FlashSetPatStr(@%d): \"%s\" (len=%d)"),
          pinfoOffset, str, strlen(str)));

  for (int i = 0; i < MAXLEN_PATSTR; ++i)
  {
    EEPROM.write((pinfoOffset + i), str[i]);
    if (!str[i]) break;
  }

  FlashDone();
}

void FlashGetPatStr(char *str)
{
  for (int i = 0; i < MAXLEN_PATSTR; ++i)
  {
    str[i] = EEPROM.read(pinfoOffset + i);
    if (!str[i]) break;
  }
  str[MAXLEN_PATSTR] = 0; // insure termination

  DBGOUT((F("FlashGetPatStr(@%d): \"%s\" (len=%d)"),
          pinfoOffset, str, strlen(str)));
}

void FlashSetPatName(char *name)
{
  DBGOUT((F("FlashSetPatName(@%d): \"%s\"  (len=%d)"),
          (pinfoOffset + FLASHLEN_PATSTR), name, strlen(name)));

  for (int i = 0; i < MAXLEN_PATNAME; ++i)
  {
    EEPROM.write((pinfoOffset + FLASHLEN_PATSTR + i), name[i]);
    if (!name[i]) break;
  }

  FlashDone();
}

void FlashGetPatName(char *name)
{
  for (int i = 0; i < MAXLEN_PATNAME; ++i)
  {
    name[i] = EEPROM.read(pinfoOffset + FLASHLEN_PATSTR + i);
    if (!name[i]) break;
  }
  name[MAXLEN_PATNAME] = 0; // insure termination

  DBGOUT((F("FlashGetPatName(@%d): \"%s\"  (len=%d)"),
          (pinfoOffset + FLASHLEN_PATSTR), name, strlen(name)));
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

void FlashSetForce(byte force)
{
  FlashSetValue(FLASHOFF_SDATA_FORCE, force);
  FlashDone();
}

short FlashGetForce(void)
{
  return FlashGetValue(FLASHOFF_SDATA_FORCE) + (FlashGetValue(FLASHOFF_SDATA_FORCE+1) << 8);
}

void FlashSetProperties(void)
{
  pPixelNutEngine->setPropertyMode(FlashGetValue(FLASHOFF_SDATA_XT_MODE));
  pPixelNutEngine->setColorProperty(FlashGetValue(FLASHOFF_SDATA_XT_HUE),
                                    FlashGetValue(FLASHOFF_SDATA_XT_WHT));
  pPixelNutEngine->setCountProperty(FlashGetValue(FLASHOFF_SDATA_XT_CNT));
}

void FlashStartup(void)
{
  FlashStart();

  #if !CLIENT_APP
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

  pPixelNutEngine->setBrightPercent(bright);
  pPixelNutEngine->setDelayPercent(delay);
  pPixelNutEngine->setFirstPosition(fpos);

  DBGOUT((F("Flash: bright=%d%%"), bright));
  DBGOUT((F("Flash: delay=%d%%"), delay));

  FlashSetProperties();

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
