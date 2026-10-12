# Vita Camera

A native C++17 camera app for homebrew PS Vita. Shoot with physical controls, read the light with live graphs, and send photographs to your phone over Wi-Fi. The interface pairs blue glass panels with the Vita's native 4:3 camera image.

![Vita Camera interface with a simulated scene](docs/ui-preview.png)

## Features

- Front and rear cameras at 640 × 480, with a preferred capture rate of 30 fps.
- Exposure compensation, white balance, ISO Auto/100/200/400 and anti-flicker settings.
- Original, Mono, Warm and Cool looks, with instant original comparison.
- Luminance and RGB histograms, waveform, highlight zebras and shadow warnings.
- 1–4× digital zoom, composition guides, and 2- or 10-second self-timers.
- Photo browsing with swipe navigation, zoom and pan.
- Local Wi-Fi sharing through a QR code and a phone browser.
- Lossless PNG originals, separate edited copies and JSON capture metadata.

The 30 fps setting improved indoor brightness in device testing. The app falls back to 60 fps if the camera cannot open or start at 30 fps. Digital zoom crops the image; the public camera API exposes no autofocus or manual lens-focus control.

## Install

You need a homebrew-enabled Vita, VitaShell and a built `vita-cam.vpk`. See [Build](#build) to create the package.

1. In VitaShell, press **START** and set **SELECT button → USB**. Choose the storage used by the app under **USB device**; for SD2Vita, select **SD2Vita**.
2. Close the menu and press **SELECT** to share USB storage.
3. Copy `dist/vita-cam.vpk` to the drive's root as `VitaCamera.vpk`, then **safely eject the drive on the computer**.
4. Press **Circle** to stop USB sharing. Open `VitaCamera.vpk` in VitaShell and install it.
5. Launch **Vita Camera** from LiveArea.

The app's title ID is `VITACAM01`. Updates preserve photographs in `ux0:data/VitaCamera/photos`. If VitaShell exports a Memory Card while SD2Vita is active, that card may appear as `uma0:` on the Vita.

## Controls

| While shooting | Action |
| --- | --- |
| R / Cross | Capture; cancel a running timer; retry a failed save |
| Up / Down | Increase / decrease exposure compensation |
| Left / Right | Previous / next graph |
| Right stick Up / Down | Zoom in / out |
| Front / Rear touch button | Switch camera |
| Square | Open photos |
| Triangle | Open settings |
| Hold Circle or touch and hold the viewfinder | Hide the interface until released |
| Hold Select | Compare with the original image |
| Start | Show controls |

**Photos:** swipe or use Left/Right to browse, use the right stick to zoom and the left stick to pan. Cross toggles 2× detail; holding Select shows the full-frame original.

**Settings:** Up/Down selects a row, Left/Right or Cross changes its value, and L/R switches tabs. Circle returns from menus or cancels a timer. L has no action while shooting.

Wait for **Photo saved** before closing the app. If a save fails, keep the app open and press Capture to retry.

## Graphs and warnings

| Graph | Reading it |
| --- | --- |
| Luminance | Dark pixels appear on the left, bright pixels on the right. Taller bars represent more pixels. |
| RGB | Separate distributions for red, green and blue. A channel crowded against the right edge can indicate clipped color detail. |
| Waveform | Horizontal position follows the image; brightness runs from bottom to top. Brighter marks represent more pixels at that position and level. |

Graphs follow the current color look and zoom crop, before warning overlays. They refresh at 10 Hz; Left/Right cycles through the three graphs and Off. Histogram heights are normalized within each graph.

Highlight zebras mark pixels with any color channel at 250/255 or above. Shadow warnings mark luminance at 5/255 or below. Both are viewing aids and stay out of saved photographs.

## Share photographs

1. Connect the Vita and phone to the same Wi-Fi network.
2. Open **Share** from the camera or photo browser.
3. Scan the QR code and open the link in the phone's browser.
4. On iPhone, touch and hold a photograph to save it to Photos. **Download PNG** saves a file; **Full original** opens the unedited, uncropped image.

Keep Share open until the transfer finishes. Circle stops the server. Transfers stay on the local network and require no cloud service or companion app. Networks that isolate connected devices can block access.

### Away from home

An iPhone's [Personal Hotspot](https://support.apple.com/en-us/111785) can provide the network: enable **Allow Others to Join**, turn on **Maximize Compatibility** when available, then connect the Vita through its system Wi-Fi settings. [Compatibility mode](https://support.apple.com/en-ca/guide/security/secfd166f620/web) uses 2.4 GHz and WPA2 on supported iPhones. Hotspot availability depends on cellular data and carrier support; this route still needs device validation.

A compatible local access point can also work without internet. For USB transfer to a computer, use VitaShell and copy `data/VitaCamera/photos`, then safely eject. Direct USB transfer to iPhone is untested. Bluetooth transfer and AirDrop are not supported by this app.

## Saved files

| Location under `ux0:data/VitaCamera/` | Contents |
| --- | --- |
| `photos/` | PNG photographs and JSON capture metadata |
| `settings.ini` | Saved preferences |
| `session.log` | Camera and application diagnostics; replaced at launch |

Every capture retains the full-frame original. Color looks and digital zoom produce a separate copy. Front-camera mirroring applies only to the preview; exports remain unmirrored.

“Original” means the camera image before app processing. It is a lossless RGB export, not photographic RAW. Metadata records selected camera modes, not measured ISO, shutter duration or delivered frame rate.

## Build

Install a native [VitaSDK](https://vitasdk.org/) toolchain and CMake. The project has been built with VitaSDK 2026.08 and GCC 15.2 on macOS ARM64.

```sh
export VITASDK=/path/to/vitasdk
export PATH="$VITASDK/bin:$PATH"
vdpm install libvita2d libpng libjpeg-turbo zlib freetype bzip2
sh scripts/build.sh
```

The package is written to `dist/vita-cam.vpk`. If `VITASDK` is unset, the script uses `.toolchain/vitasdk`. Builds and tests run natively; Docker is not required.

### Development

Host checks require CMake, a C++17 compiler, libpng and FreeType:

```sh
sh scripts/check.sh
```

Seven suites cover capture/export, NEON and scalar preview processing, text alignment, background saving, the gallery and HTTP sharing. The sharing test needs a local loopback socket. The script also creates SVG layout previews in `build-host/`.

Format project C++ sources and headers from the repository root:

```sh
git ls-files -z -- 'src/*.cpp' 'include/*.hpp' 'tests/*.cpp' | xargs -0 clang-format -i
```

The [clang-format configuration](.clang-format) uses four-space indentation and a 120-column target. The command includes tracked files in subdirectories and excludes the SDK, generated files and vendored code. See the [ClangFormat manual](https://clang.llvm.org/docs/ClangFormat.html) for editor integration.

## Documentation

- [Architecture](docs/architecture.md) — modules, frame ownership, rendering and sharing.
- [Device testing](docs/device-testing.md) — validation status, release checks and diagnostics.
- [Performance](docs/performance.md) — measured results, benchmarks and profiling.
- [Camera capabilities](vita-cam.md) — hardware reference, project scope and research directions.

## Acknowledgments

Built with [VitaSDK](https://vitasdk.org/) and [libvita2d](https://github.com/xerpi/libvita2d). The interface uses [Source Sans 3](https://github.com/adobe-fonts/source-sans) under the [SIL Open Font License](assets/fonts/LICENSE.md). QR codes use [Nayuki's QR Code generator](https://github.com/nayuki/QR-Code-generator/tree/v1.8.0/c) under its [MIT license](third_party/qrcodegen/LICENSE.txt).
