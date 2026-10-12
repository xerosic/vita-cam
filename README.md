# Vita Camera

C++17 photo app for a homebrew PS Vita. Blue glass panels, native 4:3 framing, touch controls and physical shortcuts.

![Interface preview with a simulated scene](docs/ui-preview.png)

## Features

- Rear/front camera, VGA ABGR capture; tries 60 fps, falls back to 30 fps.
- EV compensation, WB presets, requested ISO and anti-flicker.
- Original, Mono, Warm and Cool looks. Hold SELECT to compare.
- 2/10-second timer, thirds/square guides, highlight/shadow warnings.
- Luminance/RGB histograms and waveform, measured within the zoomed frame.
- Right-stick digital zoom from 1–4×. Zoomed captures save a separate copy beside the full-frame original.
- Photo browser: newest first, swipe or use Left/Right; hold Select for the full-frame original.
- Wi-Fi sharing to iPhone Safari through a QR code; no SD-card removal or companion app.
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

## Share to iPhone

1. Connect the Vita and iPhone to the same Wi-Fi network, or connect the Vita to your iPhone's Personal Hotspot.
2. Tap **Share** on the camera or photo-browser screen.
3. Scan the QR code with iPhone Camera and open the link in Safari.
4. Touch and hold a photo to save it to Photos. **Download PNG** saves a file; **Full original** opens the unedited, uncropped image.

Keep Share open until the transfer finishes. Circle stops the local server. Sharing exposes only completed app photographs through a new session link each time. Networks that isolate connected devices will block this connection. If the Vita cannot see an iPhone hotspot, enable [Maximize Compatibility](https://support.apple.com/en-us/119837) when available.

Bluetooth photo transfer is not implemented: [Sony's supported Bluetooth profiles](https://manuals.playstation.net/document/en/psvita/settings/bluetooth.html) do not include a file-transfer profile. Wi-Fi is the supported transfer path.

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

QR codes use [Nayuki's QR Code generator v1.8.0](https://github.com/nayuki/QR-Code-generator/tree/v1.8.0/c), bundled under its [MIT license](third_party/qrcodegen/LICENSE.txt).

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

Checks exercise capture/export pixels, zoom crops, graph sampling, preferences, timers and touch hits. Worker tests cover ownership, queue limits, retry and shutdown. Gallery tests cover grouping/order and incomplete files. HTTP tests use a loopback port and check download bytes, path restrictions, session rotation and interruptible shutdown. The same C++ drawing code produces `build-host/*.svg` for layout inspection with a simulated scene. `build-host/sharing_preview` serves generated sample images for browser testing; press Enter to stop it.

Hardware checks and limitations: [docs/device-testing.md](docs/device-testing.md). Design and implementation: [docs/architecture.md](docs/architecture.md). Original brief: [vita-cam.md](vita-cam.md).
