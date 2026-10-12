#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
cmake -S . -B build-host -DVITACAM_HOST=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host --parallel
ctest --test-dir build-host --output-on-failure
for screen in camera settings view review help clean error rgb waveform share offline; do
    build-host/ui_snapshot "build-host/$screen.svg" "$screen"
done
