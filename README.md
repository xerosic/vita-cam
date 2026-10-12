# Vita Camera

C++17 photo app for a homebrew PS Vita. Blue glass panels, native 4:3 framing, touch controls and physical shortcuts.

![Interface preview with a simulated scene](docs/ui-preview.png)

## Features

- Rear/front camera, VGA ABGR capture; tries 60 fps, falls back to 30 fps.
- EV compensation, WB presets, requested ISO and anti-flicker.
- Original, Mono, Warm and Cool looks. Hold SELECT to compare.
- 2/10-second timer, thirds/square guides, histogram, highlight/shadow warnings.
- Luminance/RGB histograms and waveform, measured within the zoomed frame.
- Right-stick digital zoom from 1–4×. Zoomed captures save a separate copy beside the full-frame original.
- Photo browser: newest first, swipe or use Left/Right; hold Select for the full-frame original.
- Mirrored selfie preview; exports remain unmirrored.
- Lossless 640 × 480 PNG originals, optional edited copies and JSON settings.
- Background saving, retained failed captures for retry, persistent preferences.

Files live in `ux0:data/VitaCamera/photos`. Color looks never overwrite the original. Camera settings affect both versions; “original” means the delivered camera image before app processing, not RAW. Guides, warnings and UI stay out of exports.

## Controls

| Control | Action |
| --- | --- |
| R / Cross | Shutter; cancel timer; retry failed save |
| Left / Right | EV |
| Up / Down | Previous / next graph |
| Right stick Up / Down | Zoom in / out (1–4×) |
| L | Switch camera |
| Square | Photo browser |
| Triangle | Settings |
| Circle | Clean view / back / cancel timer |
| Hold Select | Compare original |
| Start | Controls |
| Settings: Up / Down | Select row |
| Settings: Left / Right / Cross | Change value |
| Settings: L / R | Switch tab |
| Review: Cross | Toggle 2x detail |
| Review: Left / Right or swipe | Previous / next photo |
| Review: left stick | Pan when zoomed |

Touch the controls directly; tap the viewfinder for clean view. Wait for **Photo saved** before closing the app. If saving fails, keep the app open and press Capture to retry the retained frame.

## Native build

Install [VitaSDK](https://vitasdk.org/) for your host. This project uses the 2026.08 series and requires `libvita2d`, `libpng`, `libjpeg-turbo`, `zlib`, `freetype` and `bzip2`:

```sh
export VITASDK=/path/to/vitasdk
export PATH="$VITASDK/bin:$PATH"
vdpm install libvita2d libpng libjpeg-turbo zlib freetype bzip2
sh scripts/build.sh
```

Without `VITASDK`, the script uses `.toolchain/vitasdk` in this repository. Output: `dist/vita-cam.vpk`. No Docker is used for builds or tests.

Assets are included. To regenerate them, install Pillow in a virtual environment and run `scripts/artwork.py`.

The UI uses 4× MSAA and [Source Sans 3](https://github.com/adobe-fonts/source-sans), rasterized at each text size. Its [OFL license](assets/fonts/LICENSE.md) ships with the app.

## Install

1. In VitaShell, press START, set **SELECT button → USB**, close the menu and press SELECT.
2. Set **USB device** to the storage you use (for example, SD2Vita). Copy `dist/vita-cam.vpk` to that drive's root as `VitaCamera.vpk`. Eject the drive on the computer.
3. Press Circle on the Vita to stop sharing. Find `VitaCamera.vpk` at the root of that storage in VitaShell and install it. An exported Memory Card may be mounted as `uma0:` rather than `ux0:` when SD2Vita is active.
4. Launch **Vita Camera** from LiveArea.

Updating the same title (`VITACAM01`) preserves photographs in the separate data directory.

## Checks

With CMake, a native C++ compiler and libpng:

```sh
sh scripts/check.sh
```

Checks exercise EV bounds, WB enumeration, invalid preferences, timers, pixel transforms, histogram/clipping, PNG pixel equality, collision handling, write failures and touch hit testing. Worker tests cover frame ownership, queue limits, failed-save retry and shutdown draining. The same C++ drawing code produces `build-host/*.svg` for layout inspection, using a clearly labeled simulated scene.

Hardware checks and limitations: [docs/device-testing.md](docs/device-testing.md). Design and implementation: [docs/architecture.md](docs/architecture.md). Original brief: [vita-cam.md](vita-cam.md).
