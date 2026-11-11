#!/usr/bin/env sh
# Compile portable TX modules to the instruction-set targets of RTL872xCS:
# KM0 ~ Cortex-M23, KM4 ~ Cortex-M33.
# This uses test stub ameba_soc.h: NOT an SDK build, not device firmware!
set -eu
cd "$(dirname "$0")/.."
: "${CLANG:=clang}"
command -v "$CLANG" >/dev/null 2>&1 || { echo "Clang unavailable" >&2; exit 2; }
for spec in 'cortex-m23 arm-km0-m23' 'cortex-m33 arm-km4-m33'; do
    set -- $spec
    cpu=$1
    target=$2
    mkdir -p "build/$target"
    for name in gwir_amebad_tx_codec gwir_amebad_tx_fifo gwir_amebad_tx_vendor gwir_amebad_rx_fifo gwir_amebad_rx_vendor gwir_amebad_rx_irq gwir_amebad_bus gwir_amebad_vendor_bus; do
        "$CLANG" --target=arm-none-eabi -mcpu="$cpu" -mthumb \
          -mfloat-abi=soft -ffreestanding -fno-builtin -O2 \
          -std=c11 -Wall -Wextra -Werror \
          -Itests/stubs -c "firmware/realtek/$name.c" \
          -o "build/$target/$name.o"
    done
    echo "PASS compile-only $cpu with test stub (NOT linked/SDK-tested)"
done
