/*
 * Memory Spaces Definitions for STM32H533xE (also STM32H523xE).
 *
 * FLASH: 512K at 0x08000000 (two banks of 256K, 8K sectors).
 * RAM: SRAM1 (128K), SRAM2 (80K) and SRAM3 (64K) are contiguous at
 * 0x20000000, 272K in total. TrustZone is off, so the non-secure aliases
 * are used.
 *
 * The first 0x400 bytes of RAM are kept free for the copy of the vector
 * table that Interrupt::connectHandler() makes at SRAM_BASE. The table has
 * 149 entries (596 bytes). VTOR needs the table aligned to the next power of
 * two of its size, which is 1024 bytes.
 *
 * The values below can be addressed in further linker scripts
 * using functions like 'ORIGIN(RAM)' or 'LENGTH(RAM)'.
 */

MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000400, LENGTH = 272K-0x400
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
