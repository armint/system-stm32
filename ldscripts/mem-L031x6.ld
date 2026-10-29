/*
 * Memory Spaces Definitions for STM32L031x6.
 *
 * FLASH: 32K at 0x08000000. RAM: 4K at 0x20000000. The part has 8K of
 * SRAM, but on revision A only 0x20000000-0x20000FFF works (erratum 2.1.5,
 * STM32L031 errata sheet DocID027842, no workaround). Later revisions can
 * use 8K, but this script keeps 4K for all revisions.
 *
 * FLASH parameters. Give them with -Wl,--defsym=<symbol>=<bytes> before the
 * -T options; ld reads a --defsym after them too late and silently uses the
 * default:
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
__flash_reserved_start = 0x08000000 + 32K - (DEFINED(__flash_reserved_end) ? __flash_reserved_end : 0);

MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 4K
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
