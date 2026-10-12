# Device testing

Vita Camera is tested through host regression suites, layout previews and a homebrew-enabled PS Vita. This document separates confirmed behavior from checks that still require hardware validation.

## Current validation

Current release: **0.1.8**, packaged as **01.08**, title ID **VITACAM01**. The native build uses VitaSDK 2026.08 and GCC 15.2 on macOS ARM64.

| Area | Evidence |
| --- | --- |
| Launch and capture | Confirmed on device after the 01.01 pthread linking fix. |
| Curves and text | Improved native rendering confirmed with 01.02; host tests verify text centering within one pixel. |
| iPhone sharing | Transfer over shared Wi-Fi confirmed with 01.03. Mobile WebKit checks cover pages, image viewing and downloads. |
| Preview processing | NEON and scalar outputs match the reference across looks, mirroring, warnings and padded texture strides. |
| Responsiveness | Smooth preview confirmed with 01.04; historical measurements are in [Performance](performance.md). |
| Compact controls | Shared layout previews cover the 01.05 button surfaces, signs, chevrons and thumbnail padding. Native appearance is not separately signed off. |
| Hold gestures | Host tests cover release, overlapping holds, timer cancellation and menu transitions introduced in 01.07. Physical checks remain open. |
| Indoor exposure | The 30 fps preference in 01.08 resolved the reported dark indoor photographs. Noise and motion blur were not assessed separately. |
| Build and package | Native compilation, pthread link checks and all seven host suites pass. Installed 01.08 files were verified and SD2Vita safely ejected. |

The indoor result is based on device feedback. Post-update settings and camera-rate logs have not been retrieved. Hotspot sharing, direct USB transfer to iPhone and broad hardware-revision compatibility remain unverified.

## Release checklist

### Capture and recovery

1. Launch, take a photograph with R, and wait for **Photo saved**. Open the result in the gallery.
2. Switch cameras with the Front/Rear touch button. Check preview orientation and unmirrored exports.
3. Cycle EV, white balance, ISO and anti-flicker. Unsupported values should be skipped without repeated popups.
4. Capture with a color look and digital zoom. Verify the full-frame original, edited copy and JSON metadata; saved images must exclude guides and warnings.
5. Exercise both timers, cancellation, restart and suspend/resume. Confirm preferences persist and the camera recovers.
6. For rate or exposure changes, compare the same indoor scene at ISO Auto, EV 0 and Original after allowing exposure to settle. Check brightness, noise and motion blur on both cameras.

A successful `open value=30 result=0x00000000` log entry and `requested.fps` of `30` in the photo JSON identify the accepted mode. Neither measures the delivered frame rate or shutter duration.

### Interface and gallery

1. Check label alignment, exposure signs, tabs, shutter and gallery controls at 960 × 544. Capture reference screenshots with **PS + START**.
2. Verify Up/Down changes EV, Left/Right cycles graphs, and L does nothing while shooting.
3. Hold and release Circle and the viewfinder, separately and together. The UI must return when both are released. A Circle press used for Back or timer cancellation must not also hide the UI.
4. Browse photographs with Left/Right and swipes. Test right-stick zoom, left-stick pan and Select's original comparison.
5. Pan across bright and dark areas with each warning mode. Repeat with Waveform and a color look. Use the [profiler](performance.md#vita-timings) for comparable measurements.

### Sharing

1. Open Share on a common Wi-Fi network, scan the QR code and save a photograph on iPhone.
2. Check pagination, full originals and file downloads.
3. Leave Share during a transfer, then start a new session. Verify the old link stops working.
4. Test connection loss, Retry and suspension. Repeat through an iPhone hotspot when available.

## Collecting diagnostics

Close Vita Camera and enable VitaShell USB sharing for the storage containing `ux0:`. Retrieve only the files needed for the test:

| Path on the exported storage | Purpose |
| --- | --- |
| `data/VitaCamera/session.log` | Startup, camera settings, errors and optional timing samples |
| `data/VitaCamera/settings.ini` | Saved preferences |
| `data/VitaCamera/photos/` | Selected test photographs and their JSON metadata |
| `picture/SCREENSHOT/` | Relevant PS + START screenshots |

`session.log` is replaced at every launch. Copy it before reopening the app. With SD2Vita active, an exported Memory Card may correspond to `uma0:` instead of `ux0:`.

After copying or updating files, verify the transfer and **safely eject the drive on the computer before stopping USB sharing**.

Keep logs, photographs, dumps, matching debug ELFs and rollback packages in ignored local directories. The development workspace uses `build/device-logs/`, `build/device-builds/<version>/` and `build/device-backup/<version>/`; these artifacts do not belong in the public repository.

## Hardware findings

| Finding | Implementation consequence |
| --- | --- |
| Weak pthread references caused the initial startup failure. | Link the complete pthread archive and verify symbols before packaging. |
| WB modes 0, 1 and 2 were accepted; mode 4 returned `0x802e0000`. | Skip parameter/range rejections generically rather than hard-coding exclusions. |
| Auto and 50 Hz anti-flicker were accepted; 60 Hz was rejected in the recorded test. | Treat availability as device-dependent. Visual effectiveness still needs controlled testing. |
| ISO Auto, 100, 200 and 400 were accepted in recorded tests. | Expose selected modes without claiming calibrated sensitivity measurements. |
| Dark indoor captures improved after switching the preferred request from 60 to 30 fps. | Keep 30 fps as the default, with 60 fps as a compatibility fallback. |

API success establishes that a setting was accepted, not its photographic effect. Stock-camera color matching, actual exposure values and behavior across camera modules need further comparison. The [capability brief](../vita-cam.md) covers those limits and future experiments.
