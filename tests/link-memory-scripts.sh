#!/bin/sh
# Links a stub image with each memory script in ldscripts/ and sections.ld,
# behind an 8 KB offset with 1 KB reserved at the end. Then checks the FLASH
# origin and __flash_reserved_start. A 0x100 offset must fail, because the
# vector table is then not aligned for VTOR.
# Usage: tests/link-memory-scripts.sh [<toolchain bin directory>]
set -eu
bin=${1:+$1/}
root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
printf 'int g_pfnVectors[4] __attribute__((section(".isr_vector"), used));\nvoid _start(void) { for(;;); }\n' > "$tmp/stub.c"

link() {
    "${bin}arm-none-eabi-gcc" -mcpu=cortex-m0plus -mthumb -nostartfiles -nostdlib \
        -Wl,--no-warn-rwx-segments -o "$tmp/stub.elf" "$tmp/stub.c" "$@" \
        -L"$root/ldscripts" -Tlibs.ld -Tsections.ld
}

for script in "$root"/ldscripts/mem-*.ld; do
    name=$(basename "$script")
    link -Wl,--defsym=__flash_offset=0x2000 -Wl,--defsym=__flash_reserved_end=0x400 -T"$name"
    "${bin}arm-none-eabi-nm" "$tmp/stub.elf" > "$tmp/stub.nm"
    grep -q '^08002000 [DT] g_pfnVectors$' "$tmp/stub.nm" || { echo "$name: wrong FLASH origin"; exit 1; }
    grep -q ' A __flash_reserved_start$' "$tmp/stub.nm" || { echo "$name: no __flash_reserved_start"; exit 1; }
    echo "$name: ok"
done

if link -Wl,--defsym=__flash_offset=0x100 -Tmem-WLE5xC.ld 2>/dev/null; then
    echo "a 0x100 offset links, but it must fail"
    exit 1
fi
echo "0x100 offset: fails as expected"
