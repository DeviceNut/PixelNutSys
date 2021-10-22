/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"
#include "flash.h"
#include <EEPROM.h>

#if (FLASHOFF_PATTERN_END > EEPROM_BYTES)
#error("Not enough flash space to store external pattern strings");
#endif

static byte valOffset = FLASHOFF_STRAND_DATA;
static uint16_t strOffset = FLASHOFF_PATTERN_START;

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
  int oldval = (valOffset - FLASHOFF_STRAND_DATA) / FLASH_SEG_LENGTH;
  valOffset = FLASHOFF_STRAND_DATA + (strandindex * FLASH_SEG_LENGTH);
  return oldval;
}

#if CLIENT_APP

void FlashSetName(char *name)
{
  DBGOUT((F("FlashSetName: \"%s\""), name));

  for (int i = 0; i < MAXLEN_DEVICE_NAME; ++i)
    EEPROM.write(i, name[i]);

  FlashDone();
}

void FlashGetName(char *name)
{
  for (int i = 0; i < MAXLEN_DEVICE_NAME; ++i)
    name[i] = EEPROM.read(i);

  name[MAXLEN_DEVICE_NAME] = 0;

  DBGOUT((F("FlashGetName: \"%s\""), name));
}

#endif // CLIENT_APP

void FlashSetStr(char *str, int offset)
{
  DBGOUT((F("FlashSetStr(@%d): \"%s\" (len=%d)"), (strOffset + offset), str, strlen(str)));

  for (int i = 0; ; ++i)
  {
    if ((strOffset + offset + i) >= EEPROM_BYTES) break; // prevent overrun
    EEPROM.write((strOffset + offset + i), str[i]);
    if (!str[i]) break;
  }

  FlashDone();
}

void FlashGetStr(char *str)
{
  for (int i = 0; ; ++i)
  {
    if ((i >= (STRLEN_PATSTR-1)) ||
        ((strOffset + i) >= EEPROM_BYTES))
         str[i] = 0; // prevent overrun
    else str[i] = EEPROM.read(strOffset + i);
    if (!str[i]) break;
  }

  DBGOUT((F("FlashGetStr(@%d): \"%s\" (len=%d)"), strOffset, str, strlen(str)));
}

void FlashSetPatNum(byte pattern) { FlashSetValue(FLASH_SEG_PATNUM, pattern); FlashDone(); }

void FlashSetBright()    { FlashSetValue(FLASH_SEG_BRIGHTNESS, pPixelNutEngine->getMaxBrightness());  FlashDone(); }
void FlashSetDelay()     { FlashSetValue(FLASH_SEG_DELAYMSECS, pPixelNutEngine->getDelayOffset());    FlashDone(); }
void FlashSetFirst()     { FlashSetValue(FLASH_SEG_FIRSTPOS,   pPixelNutEngine->getFirstPosition());  FlashDone(); }

void FlashSetXmode(bool enable) { FlashSetValue(FLASH_SEG_XT_MODE, enable);  FlashDone(); }

void FlashSetExterns(uint16_t hue, byte wht, byte cnt)
{
  FlashSetValue(FLASH_SEG_XT_HUE,   hue&0xFF);
  FlashSetValue(FLASH_SEG_XT_HUE+1, hue>>8);
  FlashSetValue(FLASH_SEG_XT_WHT,   wht);
  FlashSetValue(FLASH_SEG_XT_CNT,   cnt);
  FlashDone();
}

void FlashSetForce(short force)
{
  FlashSetValue(FLASH_SEG_FORCE, force);
  FlashSetValue(FLASH_SEG_FORCE+1, (force >> 8));
  FlashDone();
}

short FlashGetForce(void)
{
  return FlashGetValue(FLASH_SEG_FORCE) + (FlashGetValue(FLASH_SEG_FORCE+1) << 8);
}

void FlashSetProperties(void)
{
  uint16_t hue = FlashGetValue(FLASH_SEG_XT_HUE) + (FlashGetValue(FLASH_SEG_XT_HUE+1) << 8);

  pPixelNutEngine->setPropertyMode(      FlashGetValue(FLASH_SEG_XT_MODE));
  pPixelNutEngine->setColorProperty(hue, FlashGetValue(FLASH_SEG_XT_WHT));
  pPixelNutEngine->setCountProperty(     FlashGetValue(FLASH_SEG_XT_CNT));
}

  void FlashStartup(void)
{
  FlashStart();

  #if !CLIENT_APP
  curPattern = FlashGetValue(FLASH_SEG_PATNUM);
  if (!curPattern || (curPattern > codePatterns))
    curPattern = 1; // 1..codePatterns
  DBGOUT((F("Flash: pattern=#%d"), curPattern));
  #endif
 
  byte bright = FlashGetValue(FLASH_SEG_BRIGHTNESS);
  if (bright == 0) FlashSetValue(FLASH_SEG_BRIGHTNESS, bright=MAX_BRIGHTNESS); // set to max if 0

  int8_t delay = (int8_t)FlashGetValue(FLASH_SEG_DELAYMSECS);
  if ((delay < -DELAY_RANGE) || (DELAY_RANGE < delay)) delay = 0; // set to min if out of range

  int16_t fpos = FlashGetValue(FLASH_SEG_FIRSTPOS);

  pPixelNutEngine->setMaxBrightness(bright);
  pPixelNutEngine->setDelayOffset(delay);
  pPixelNutEngine->setFirstPosition(fpos);

  DBGOUT((F("Flash: brightness=%d%%"), bright));
  DBGOUT((F("Flash: delay=%d msecs"), delay));

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
