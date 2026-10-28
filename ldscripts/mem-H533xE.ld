/*
 * Memory Spaces Definitions for STM32H533xE. The STM32H523xE has the same
 * memory map.
 *
 * FLASH: 512K at 0x08000000 (two banks of 256K, 8K sectors).
 * RAM: SRAM1 (128K), SRAM2 (80K) and SRAM3 (64K) are contiguous at
 * 0x20000000, 272K in total. TrustZone is off, so the non-secure aliases
 * are used. Do not reset the backup domain (VSWRST) after start-up: it also
 * erases SRAM2 (ES0621 section 2.2.6).
 *
 * The values below can be addressed in further linker scripts
 * using functions like 'ORIGIN(RAM)' or 'LENGTH(RAM)'.
 */

MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 272K
  CCMRAM (xrw) : ORIGIN = 0x00000000, LENGTH = 0
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
  EEPROM (rx) : ORIGIN = 0x00000000, LENGTH = 0
  FLASHB1 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB0 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB1 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB2 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB3 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  MEMORY_ARRAY (xrw)  : ORIGIN = 0x00000000, LENGTH = 0
}
