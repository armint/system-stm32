/*
 * Memory Spaces Definitions for STM32F103xB (medium density).
 *
 * FLASH: 128K at 0x08000000. RAM: 20K at 0x20000000.
 *
 * The values below can be addressed in further linker scripts
 * using functions like 'ORIGIN(RAM)' or 'LENGTH(RAM)'.
 */

MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
  CCMRAM (xrw) : ORIGIN = 0x00000000, LENGTH = 0
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 128K
  EEPROM (rx) : ORIGIN = 0x00000000, LENGTH = 0
  FLASHB1 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB0 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB1 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB2 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB3 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  MEMORY_ARRAY (xrw)  : ORIGIN = 0x00000000, LENGTH = 0
}
