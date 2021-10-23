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
#define FLASHLEN_STRAND_DATA          12

// offsets within each strand space:
#define FLASHOFF_SDATA_PATNUM       0
#define FLASHOFF_SDATA_PC_BRIGHT    1
#define FLASHOFF_SDATA_PC_DELAY     2
#define FLASHOFF_SDATA_FIRSTPOS     3  // 2 bytes
#define FLASHOFF_SDATA_XT_MODE      5
#define FLASHOFF_SDATA_XT_HUE       6  // 2 bytes
#define FLASHOFF_SDATA_XT_WHT       8
#define FLASHOFF_SDATA_XT_CNT       9
#define FLASHOFF_SDATA_FORCE        10 // 2 bytes

#if CLIENT_APP
#define FLASHOFF_STRAND_DATA        MAXLEN_DEVICE_NAME
#define FLASHLEN_PATSTR             STRLEN_PATSTR
#define FLASHLEN_PATNAME            STRLEN_PATNAME
#else
#define FLASHOFF_STRAND_DATA        0
#define FLASHLEN_PATSTR             0
#define FLASHLEN_PATNAME            0
#endif

#define FLASHOFF_PINFO_START  (FLASHOFF_STRAND_DATA + (STRAND_COUNT * FLASHLEN_STRAND_DATA))
#define FLASHOFF_PINFO_END    (FLASHOFF_PINFO_START + (STRAND_COUNT * (FLASHLEN_PATSTR + FLASHLEN_PATNAME)))

#if (FLASHOFF_PINFO_END > EEPROM_BYTES)
#error("EEPROM not large enough")
#endif
#define EEPROM_FREE_START  FLASHOFF_PINFO_END
#define EEPROM_FREE_BYTES  (EEPROM_BYTES - EEPROM_FREE_START)

extern void FlashSetValue(uint16_t offset, byte value);
extern byte FlashGetValue(uint16_t offset);
#if CLIENT_APP
extern void FlashSetDevName(char *name);
extern void FlashGetDevName(char *name);
extern void FlashSetPatStr(char *str);
extern void FlashGetPatStr(char *str);
extern void FlashSetPatName(char *name);
extern void FlashGetPatName(char *name);
#endif
extern byte FlashSetStrand(byte strandindex);
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
