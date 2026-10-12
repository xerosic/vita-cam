# Device checks

## Build verification

- Native macOS ARM64 VitaSDK 2026.08; GCC 15.2.0.
- ARM executable links and packages as `VITACAM01`.
- Host capture checks pass, including RGB PNG round-trip equality.
- Camera and settings layouts inspected using shared C++ SVG output.
- VPK copied to the connected Vita's 8 GB Memory Card as `VitaCamera.vpk` at the root; source and destination SHA-256 hashes match. The initial copy also exists in `data/vita-cam.vpk`.
- With SD2Vita, the exported Memory Card may be secondary storage. Confirm the VitaShell mount before looking under `ux0:`.
- Initial on-device launch crashed. The original ARM executable omitted weakly referenced pthread functions, including `pthread_cancel` and condition-variable operations.
- Linking the complete pthread archive resolves those functions. A post-link check rejects unresolved pthread functions; it fails against the original executable and passes against the corrected build.
- Startup logging now begins before graphics and worker construction. Target binaries retain debug symbols locally for crash analysis.
- Corrected package version `01.01` (app `0.1.1`) copied to both VPK locations on the shared Memory Card; hashes match the build.
- SD2Vita dump `psp2core-1791805962-0x00001a2f3d-eboot.bin.psp2dmp` confirms an uncaught exception from `CaptureWriter` → `std::thread::_M_start_thread` → `__cxa_throw` → terminate → abort. The reported undefined instruction is newlib's intentional abort trap, not an unsupported CPU instruction.
- The installed executable's SHA-256 matched the preserved original VPK. Stack addresses were adjusted for the dump's relocated module base and resolved against that original ELF.
- Corrected executable and version metadata installed directly to `ux0:app/VITACAM01`; verified against the new VPK. Matching `VitaCamera.vpk` copied to SD2Vita's root. SD2Vita safely ejected before the launch check.
- Corrected on-device launch and photo capture confirmed by the user.
- Retrieved three 960 × 544 PS + START screenshots from `picture/SCREENSHOT` on SD2Vita (2026-10-12, 14:28). Camera and both Settings tabs show stair-stepped curves and uneven scaled PGF text. Local references are in ignored `build/device-screenshots/before/`.
- Version `01.02` adds 4× MSAA, triangle-based rounded corners/rings/lines, per-size FreeType text and filtering for loaded thumbnails. The user confirmed improved on-device appearance.
- Installed `01.02` executable, font, license and metadata directly to `app/VITACAM01`; all bytes verified against the VPK. Matching `VitaCamera.vpk` copied to SD2Vita's root; volume safely ejected. Working `01.01` files and VPK preserved in ignored `build/device-backup/01.01/`.
- The `01.02` device log confirms WB modes 1 and 2 succeed; mode 4 returns `SCE_CAMERA_ERROR_PARAM` (`0x802e0000`). Setting cycles now skip parameter/range rejections rather than getting stuck on that value. No replacement WB ID is guessed.
- Version `01.03` adds graph cycling, 1–4× digital zoom with separate exports, gallery browsing and QR-based Wi-Fi sharing. Native build and all four host suites pass. Mobile WebKit checks confirm a 393-pixel layout without horizontal overflow, 44-pixel download targets, pagination, full-size PNG viewing and successful PNG downloads. Actual Vita-to-iPhone transfer and control behavior remain to be tested.
- Installed `01.03` executable, version metadata and licenses to `app/VITACAM01`, with matching `VitaCamera.vpk` at SD2Vita's root. All copied files verified; SD2Vita safely ejected. Rollback files are in `build/device-backup/01.02/`; matching debug ELF and VPK are preserved in `build/device-builds/01.03/`.

## On the Vita

1. Launch; confirm rear preview is upright and responsive.
2. Take a photo with R. Confirm **Photo saved**, then Square for review.
3. Browse older photos with Left/Right and swipe. Zoom with the right stick and pan with the left stick; Circle returns.
4. Switch with L. Confirm front preview orientation and natural exported orientation.
5. Adjust EV and WB; compare scene response and `session.log` setter/getter results.
6. Select Warm, take a photo, check original PNG, Warm PNG and JSON in `photos`.
7. Enable guides and warnings; verify they appear only in preview.
8. Try both timers and cancel with Circle; no capture should occur after cancellation.
9. Suspend/resume and return from LiveArea; preview should recover.
10. Reopen; verify settings and last shot persist.
11. Cycle Up/Down through Luminance, RGB, Waveform and Off. Confirm the graph follows the zoomed composition.
12. Zoom with the right stick and capture. Verify the `-Zoom` copy matches the framing and its original retains the full view.
13. In Settings, cycle white balance in both directions; unsupported modes should be skipped without repeated popups.
14. Open Share on the same Wi-Fi network as the iPhone. Scan the QR code, view an older photo, save it to Photos and compare downloaded PNG bytes if exported back to the computer.
15. Exit Share during a download; the app should return promptly. Test Wi-Fi loss and suspension, then Retry with a new session code.

## UI iteration

Capture the camera and Settings with PS + START, then share SD2Vita through VitaShell. Retrieve the relevant PNGs under `picture/SCREENSHOT`; inspect at native 960 × 544 resolution. Check small labels, title text, button corners, shutter ring and thumbnail edges. Confirm the camera remains responsive and R still saves a photograph. Safely eject the volume before stopping USB sharing. The SVG preview does not reproduce the Vita's GPU/font rendering.

## Limits

Stock-camera appearance, supported frame rates and control behavior need comparison on the actual unit. API success alone is not photographic calibration. No manual focus, shutter-duration control, RAW claim, AE/AWB lock, video, bracketing, intervalometer or native Photos import is implemented. Those belong to the later research work in the brief.

The app stores only requested ISO/WB/EV modes. It does not invent EXIF exposure measurements. A scene can change while auto exposure settles. Saved front-camera images are unmirrored regardless of the preview preference.

On an error, re-enable VitaShell USB and inspect only `data/VitaCamera/session.log`, `settings.ini` and the relevant photo/JSON pair. Do not copy unrelated device files.
