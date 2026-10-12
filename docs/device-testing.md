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
| 01.04 performance | User confirmed smooth preview. Device log median: 29.9 fps with both warnings and graphs off; 28.3 fps with highlights and Waveform. Details in [performance.md](performance.md). |
| 01.05 UI polish | Shared WebKit previews check continuous button rims, vector signs/chevrons, balanced thumbnail padding, centered empty states and hotspot instructions. Native appearance awaits confirmation. |
| 01.06 ISO label | Native build, package integrity and generated Settings label verified. Included in 01.07. |
| 01.07 controls | Up/Down adjusts EV; Left/Right cycles graphs; touch switches cameras; clean view is held. Host checks cover release, combined holds, timer cancellation and menu transitions. Device check pending. |
| 01.08 camera rate | Requests 30 fps before the 60 fps fallback. Native build, pthread link check and all seven host suites pass. Deployment and indoor comparison pending. |

Build environment: macOS ARM64, VitaSDK 2026.08, GCC 15.2. Title ID: `VITACAM01`; current package: `01.08` (app `0.1.8`). The settings label is simply **ISO**; ISO behavior and metadata format are unchanged.

The 01.04 profiling build was copied to the connected SD2Vita and verified against the VPK, then the volume was safely ejected. The matching ELF/package are in `build/device-builds/01.04-profile/`; 01.03 rollback files are in `build/device-backup/01.03/`.

The 01.05 release build (profiling disabled) is installed and verified against the packaged files, with `VitaCamera.vpk` at the SD2Vita root. The volume was safely ejected. Matching debug files are in `build/device-builds/01.05/`; the previous working files are in `build/device-backup/01.04-profile/`.

The 01.07 release replaces 01.05 on the device. Installed files and the root VPK were verified against the build, then SD2Vita was safely ejected. Debug files are in `build/device-builds/01.07/`, with 01.05 rollback files in `build/device-backup/01.05/`. All seven host suites pass; physical control checks remain pending.

## Release checks on Vita

1. Launch and pan the camera with warnings Off, Highlights, Shadows and both. Compare responsiveness in the same scene, including bright and dark areas. Repeat with Waveform and a color look enabled.
2. Check button labels, exposure signs, settings tabs, shutter and gallery controls at 960 × 544. Capture camera and Settings with **PS + START**.
3. Take a photo with R. After **Photo saved**, browse older photos using Left/Right and swipe. Zoom with the right stick; pan with the left stick; hold Select to compare the original.
4. Switch cameras using the Front/Rear touch button. Confirm L does nothing while shooting. Check preview and exported orientation; saved selfies remain unmirrored.
5. Cycle EV, WB, ISO and anti-flicker. Unsupported modes should be skipped without repeated popups.
6. Capture with a color look and digital zoom. Check the full-frame original, edited/zoomed PNG and JSON; guides and warnings must be absent from saved images.
7. Try both timers, cancel, reopen the app and suspend/resume. Confirm preferences persist and the camera recovers.
8. Share to iPhone, save a photograph, then exit Share during another transfer. Test Wi-Fi loss and Retry with a fresh session link.
9. Confirm Up/Down changes EV and Left/Right changes graphs. Hold and release Circle and the viewfinder, separately and together. Controls must return once both are released. Returning from menus and cancelling a timer with Circle must not also hide the UI.
10. For 01.08, compare the same indoor scene with 01.07 at ISO Auto, EV 0 and Original after allowing exposure to settle. Check brightness, noise and motion blur; repeat on both cameras. Verify `open value=30 result=0x00000000` in the session log and `requested.fps: 30` in the photo JSON. An accepted mode is not a measured frame rate or shutter duration.

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
- The 01.03 user report identified severe lag with both warning modes. The user confirmed smooth operation after the 01.04 renderer changes; timing results are recorded above.
- At the 60 fps request, the user reported dark indoor photographs and a noisy preview, with little visible response to EV. Version 01.08 changes only the preferred camera rate to isolate its effect; the cause remains unconfirmed.

## Limits

Stock-camera appearance, delivered frame rates and exposure behavior need device comparison. API success is not photographic calibration. Metadata records requested ISO/WB/EV modes, not measured exposure values.

Manual focus, shutter-duration control, RAW, AE/AWB lock, video, bracketing, intervalometer and native Photos import are not implemented. The [research brief](../vita-cam.md) discusses later experiments.
