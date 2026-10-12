#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
export VITASDK="${VITASDK:-$PWD/.toolchain/vitasdk}"
export PATH="$VITASDK/bin:$PATH"
if [ ! -x "$VITASDK/bin/arm-vita-eabi-g++" ]; then
    echo 'Set VITASDK to your native VitaSDK installation.' >&2
    exit 1
fi
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
mkdir -p dist
cp build/vita-cam.vpk dist/vita-cam.vpk
