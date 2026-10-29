/*
 * Memory Spaces Definitions for STM32H523xC. The STM32H533xC has the same
 * memory map.
 *
 * FLASH: 256K at 0x08000000 (two banks of 128K, 8K sectors).
 * RAM: SRAM1 (128K), SRAM2 (80K) and SRAM3 (64K) are contiguous at
 * 0x20000000, 272K in total. TrustZone is off, so the non-secure aliases
 * are used. Do not reset the backup domain (VSWRST) after start-up: it also
 * erases SRAM2 (ES0621 section 2.2.6).
 *
 * FLASH parameters. Give them with -Wl,--defsym=<symbol>=<bytes>, and give
 * the scripts with -T, not -Wl,-T. The gcc driver then passes the --defsym
 * options to ld first. ld reads a --defsym after a script too late and
 * silently uses the default:
 *   __flash_offset        Flash in front of the image, for example for a
 *                         bootloader. Default 0.
 *   __flash_reserved_end  Flash at the end that stays out of FLASH, for
 *                         example for settings. Default 0.
 *   __flash_length        Length of FLASH. Default: from the image start to
 *                         the reserved flash at the end.
 * __flash_reserved_start is the address of the reserved flash at the end.
 *
 * The values below can be addressed in further linker scripts
 * using functions like 'ORIGIN(RAM)' or 'LENGTH(RAM)'.
 */

__flash_origin = 0x08000000 + (DEFINED(__flash_offset) ? __flash_offset : 0);
__flash_reserved_start = 0x08000000 + 256K - (DEFINED(__flash_reserved_end) ? __flash_reserved_end : 0);

MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 272K
  CCMRAM (xrw) : ORIGIN = 0x00000000, LENGTH = 0
  FLASH (rx) : ORIGIN = __flash_origin, LENGTH = DEFINED(__flash_length) ? __flash_length : __flash_reserved_start - __flash_origin
  EEPROM (rx) : ORIGIN = 0x00000000, LENGTH = 0
  FLASHB1 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB0 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB1 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB2 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  EXTMEMB3 (rx) : ORIGIN = 0x00000000, LENGTH = 0
  MEMORY_ARRAY (xrw)  : ORIGIN = 0x00000000, LENGTH = 0
}
