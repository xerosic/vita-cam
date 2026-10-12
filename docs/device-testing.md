# Device checks

## Validation status

| Area | Evidence |
| --- | --- |
| Launch and photo capture | Confirmed on the connected Vita after the 01.01 threading fix. |
| Curves and text rendering | User confirmed improved appearance with 01.02. |
| iPhone sharing | User confirmed wireless photo sharing works with 01.03. Mobile WebKit checks also covered layout, pagination, image viewing and PNG downloads. |
| 01.04 preview pixels | Host NEON and scalar tests match the reference for all looks, mirror modes and warning combinations, including padded texture strides. |
| 01.04 text alignment | Actual FreeType raster bounds center labels within one pixel, including signs, capitals and descenders at multiple sizes. Shared SVG layouts inspected in WebKit. |
| Build and regression checks | Native VitaSDK ARM build and pthread link check pass. All seven host suites pass. |
| 01.04 performance | Host benchmark recorded in [performance.md](performance.md). Vita responsiveness and final native screenshots await testing. |

Build environment: macOS ARM64, VitaSDK 2026.08, GCC 15.2. Title ID: `VITACAM01`; current package: `01.04` (app `0.1.4`).

The 01.04 profiling build was copied to the connected SD2Vita and verified against the VPK, then the volume was safely ejected. The matching ELF/package are in `build/device-builds/01.04-profile/`; 01.03 rollback files are in `build/device-backup/01.03/`.

## Release checks on Vita

1. Launch and pan the camera with warnings Off, Highlights, Shadows and both. Compare responsiveness in the same scene, including bright and dark areas. Repeat with Waveform and a color look enabled.
2. Check button labels, exposure signs, settings tabs, shutter and gallery controls at 960 × 544. Capture camera and Settings with **PS + START**.
3. Take a photo with R. After **Photo saved**, browse older photos using Left/Right and swipe. Zoom with the right stick; pan with the left stick; hold Select to compare the original.
4. Switch cameras. Check preview and exported orientation; saved selfies remain unmirrored.
5. Cycle EV, WB, ISO and anti-flicker. Unsupported modes should be skipped without repeated popups.
6. Capture with a color look and digital zoom. Check the full-frame original, edited/zoomed PNG and JSON; guides and warnings must be absent from saved images.
7. Try both timers, cancel, reopen the app and suspend/resume. Confirm preferences persist and the camera recovers.
8. Share to iPhone, save a photograph, then exit Share during another transfer. Test Wi-Fi loss and Retry with a fresh session link.

For timing logs, use the [profiling build](performance.md#vita-timings). Leave each warning combination on the camera screen for at least ten seconds before changing settings.

## Retrieving evidence

Close the app, enable VitaShell USB for the storage containing `ux0:`, then retrieve only relevant files:

- `data/VitaCamera/session.log` and `settings.ini`.
- Requested captures and their JSON from `data/VitaCamera/photos/`.
- Relevant **PS + START** images from `picture/SCREENSHOT/`.

Verify transferred packages against their source, then **safely eject on the computer before stopping USB sharing**. With SD2Vita active, an exported Memory Card may instead be mounted as `uma0:`.

Local device logs, screenshots, matching debug ELFs and rollback packages stay in ignored `build/` subdirectories. Do not publish user photographs or device dumps.

## Known hardware findings

- The initial startup dump traced to `std::thread::_M_start_thread`: weak pthread references were omitted by static archive extraction. Linking the whole pthread archive fixes startup; `scripts/check-vita-link.cmake` prevents recurrence. The dump's undefined instruction was the abort trap, not an unsupported CPU instruction.
- On this Vita, WB modes 0, 1 and 2 work; SDK mode 4 returns `SCE_CAMERA_ERROR_PARAM` (`0x802e0000`). Mode cycling skips parameter/range rejections without hard-coded exclusions.
- The 01.03 user report identified severe lag with both warning modes. The 01.04 renderer removes repeated frame processing/copies and batches UI geometry; a hardware speedup is not yet measured.

## Limits

Stock-camera appearance, delivered frame rates and exposure behavior need device comparison. API success is not photographic calibration. Metadata records requested ISO/WB/EV modes, not measured exposure values.

Manual focus, shutter-duration control, RAW, AE/AWB lock, video, bracketing, intervalometer and native Photos import are not implemented. The [research brief](../vita-cam.md) discusses later experiments.
