/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#include "main.h"
#include "main/flash.h"
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
    EEPROM.write(FLASHLEN_ID + i, name[i]);
    if (!name[i]) break;
  }

  FlashDone();
}

void FlashGetDevName(char *name)
{
  for (int i = 0; i < MAXLEN_DEVICE_NAME; ++i)
  {
    name[i] = EEPROM.read(FLASHLEN_ID + i);
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
void FlashSetFirst()
{
  uint16_t val = pPixelNutEngine->getFirstPosition();
  FlashSetValue(FLASHOFF_SDATA_FIRSTPOS,   val&0xff);
  FlashSetValue(FLASHOFF_SDATA_FIRSTPOS+1, val>>8);
  FlashDone();
}

void FlashSetXmode(bool enable) { FlashSetValue(FLASHOFF_SDATA_XT_MODE, enable);  FlashDone(); }

void FlashSetExterns(uint16_t hue, byte wht, byte cnt)
{
  FlashSetValue(FLASHOFF_SDATA_XT_HUE,   (hue & 0xff));
  FlashSetValue(FLASHOFF_SDATA_XT_HUE+1, (hue >> 8));
  FlashSetValue(FLASHOFF_SDATA_XT_WHT, wht);
  FlashSetValue(FLASHOFF_SDATA_XT_CNT, cnt);
  FlashDone();
}

bool FlashStartup(void)
{
  bool doinit = false;

  FlashStart();

  char sysName[FLASHLEN_ID + 1];
  for (int i = 0; i < FLASHLEN_ID; ++i) sysName[i] = EEPROM.read(i);
  sysName[FLASHLEN_ID] = 0;

  if (strncmp(sysName, FLASHSTR_ID, FLASHLEN_ID))
  {
    DBGOUT((F("Clearing flash memory: ID=\"%s\""), FLASHSTR_ID));
    for (int i = 0; i < FLASHLEN_ID; ++i) EEPROM.write(i, FLASHSTR_ID[i]);
    for (int i = FLASHLEN_ID; i < EEPROM_BYTES; ++i) EEPROM.write(i, 0);

    doinit = true;
  }

  FlashDone();

  return doinit;
}

void FlashInitStrand(bool doinit)
{
  FlashStart();

  char devName[MAXLEN_DEVICE_NAME + 1];
  FlashGetDevName(devName);
  if (!devName[0])
  {
    FlashSetDevName((char*)DEFAULT_DEVICE_NAME);
    doinit = true;
  }

  #if DEV_PATTERNS
  curPattern = FlashGetValue(FLASHOFF_SDATA_PATNUM);
  if (!curPattern || (curPattern > codePatterns)) curPattern = 1; // 1..codePatterns
  DBGOUT((F("Flash: pattern=#%d"), curPattern));
  #endif

  byte bright = FlashGetValue(FLASHOFF_SDATA_PC_BRIGHT);
  if ((bright > MAX_BRIGHTNESS) || (doinit && !bright))
  {
    DBGOUT((F("Resetting bright: %d => %d %%"), bright, DEF_PERCENTAGE));
    FlashSetValue(FLASHOFF_SDATA_PC_BRIGHT, bright=DEF_PERCENTAGE);
  }

  byte delay = FlashGetValue(FLASHOFF_SDATA_PC_DELAY);
  if ((delay > MAX_PERCENTAGE) || (doinit && !delay))
  {
    DBGOUT((F("Resetting delay: %d => %d %%"), delay, DEF_PERCENTAGE));
    FlashSetValue(FLASHOFF_SDATA_PC_DELAY, delay=DEF_PERCENTAGE);
  }

  int16_t fpos = FlashGetValue(FLASHOFF_SDATA_FIRSTPOS);

  DBGOUT((F("Flash: bright=%d%% delay=%d%%"), bright, delay));

  pPixelNutEngine->setBrightPercent(bright);
  pPixelNutEngine->setDelayPercent(delay);
  pPixelNutEngine->setFirstPosition(fpos);

  uint16_t hue = FlashGetValue(FLASHOFF_SDATA_XT_HUE) +
                (FlashGetValue(FLASHOFF_SDATA_XT_HUE+1) << 8);

  pPixelNutEngine->setPropertyMode(      FlashGetValue(FLASHOFF_SDATA_XT_MODE));
  pPixelNutEngine->setColorProperty(hue, FlashGetValue(FLASHOFF_SDATA_XT_WHT));
  pPixelNutEngine->setCountProperty(     FlashGetValue(FLASHOFF_SDATA_XT_CNT));

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
