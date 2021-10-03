/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#pragma once

#if !defined(EEPROM_BYTES)
#if defined(__arm__) && defined(__MKL26Z64__)         // Teensy LC
#define EEPROM_BYTES            128
#elif defined(__arm__) && defined(__MK20DX256__)      // Teensy 3.2
#define EEPROM_BYTES            2048
#elif defined(ESP32)                                  // Espressif ESP32
#define EEPROM_BYTES            4096
#elif defined(__AVR__)                                // all other AVR processors
#define EEPROM_BYTES            1024
#else
#error("Unspecified processor: need EEPROM length")
#endif
#endif

// for each strand:
#define FLASH_SEG_LENGTH        12

// offsets within each strand space:
#define FLASH_SEG_PATNUM        0
#define FLASH_SEG_BRIGHTNESS    1
#define FLASH_SEG_DELAYMSECS    2
#define FLASH_SEG_FIRSTPOS      3  // 2 bytes
#define FLASH_SEG_XT_MODE       5
#define FLASH_SEG_XT_HUE        6  // 2 bytes
#define FLASH_SEG_XT_WHT        8
#define FLASH_SEG_XT_CNT        9
#define FLASH_SEG_FORCE         10 // 2 bytes

#if CLIENT_APP
#define FLASHOFF_STRAND_DATA    MAXLEN_DEVICE_NAME
#define FLASHLEN_PATTERN        STRLEN_PATTERNS
#else
#define FLASHOFF_STRAND_DATA    0
#define FLASHLEN_PATTERN        0
#endif

#define FLASHOFF_PATTERN_START  (FLASHOFF_STRAND_DATA  + (STRAND_COUNT * FLASH_SEG_LENGTH))
#define FLASHOFF_PATTERN_END    (FLASHOFF_PATTERN_START + (STRAND_COUNT * FLASHLEN_PATTERN))

#define EEPROM_FREE_START  FLASHOFF_PATTERN_END
#define EEPROM_FREE_BYTES  (EEPROM_BYTES - EEPROM_FREE_START)

extern void FlashSetValue(uint16_t offset, byte value);
extern byte FlashGetValue(uint16_t offset);
extern byte FlashSetStrand(byte strandindex);
extern void FlashSetName(char *name);
extern void FlashGetName(char *name);
extern void FlashSetStr(char *str, int offset);
extern void FlashGetStr(char *str);
extern void FlashSetBright();
extern void FlashSetDelay();
extern void FlashSetFirst();
extern void FlashSetPatNum(byte pattern);
extern void FlashSetXmode(bool enable);
extern void FlashSetExterns(uint16_t hue, byte wht, byte cnt);
extern void FlashSetForce(short force);
extern short FlashGetForce(void);
extern void FlashSetProperties(void);
extern void FlashStartup(void);
#if EEPROM_FORMAT
extern void FlashFormat(void);
#endif
