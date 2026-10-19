
#!/usr/bin/env bash
set -Eeuo pipefail
set -x

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
FIRMWARE_DIR="${ROOT_DIR}/firmware"
BUILD_DIR="${ROOT_DIR}/build/firmware"

for tool in avr-g++ avr-objcopy avr-size; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "ERRO: ferramenta '${tool}' não encontrada." >&2
        echo "Instale o toolchain AVR e tente novamente." >&2
        exit 127
    fi
done

mkdir -p "${BUILD_DIR}"

avr-g++ \
    -mmcu=atmega328p \
    -DF_CPU=16000000UL \
    -std=gnu++17 \
    -Os \
    -g \
    -Wall -Wextra \
    -ffunction-sections -fdata-sections \
    -I "${FIRMWARE_DIR}/include" \
    "${FIRMWARE_DIR}/blink.cpp" \
    "${FIRMWARE_DIR}/src/Arduino.cpp" \
    -Wl,--gc-sections \
    -o "${BUILD_DIR}/blink.elf"

avr-objcopy \
    -O ihex \
    -R .eeprom \
    "${BUILD_DIR}/blink.elf" \
    "${BUILD_DIR}/blink.hex"

avr-size --mcu=atmega328p "$BUILD_DIR/blink.elf"

echo "Firmware compilado:"
echo "  ELF: ${BUILD_DIR}/blink.elf"
echo "  HEX: ${BUILD_DIR}/blink.hex"
