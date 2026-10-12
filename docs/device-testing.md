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
- Crash dump inspection and corrected on-device launch: awaiting access to the Vita's `ux0:` storage; the currently shared Memory Card contains neither the installed app nor its dump.

## On the Vita

1. Launch; confirm rear preview is upright and responsive.
2. Take a photo with R. Confirm **Photo saved**, then Square for review.
3. Use Cross for 2x detail and pan with the D-pad; Circle returns.
4. Switch with L. Confirm front preview orientation and natural exported orientation.
5. Adjust EV and WB; compare scene response and `session.log` setter/getter results.
6. Select Warm, take a photo, check original PNG, Warm PNG and JSON in `photos`.
7. Enable guides and warnings; verify they appear only in preview.
8. Try both timers and cancel with Circle; no capture should occur after cancellation.
9. Suspend/resume and return from LiveArea; preview should recover.
10. Reopen; verify settings and last shot persist.

## Limits

Stock-camera appearance, supported frame rates and control behavior need comparison on the actual unit. API success alone is not photographic calibration. No manual focus, shutter-duration control, RAW claim, AE/AWB lock, video, bracketing, intervalometer or native Photos import is implemented. Those belong to the later research work in the brief.

The app stores only requested ISO/WB/EV modes. It does not invent EXIF exposure measurements. A scene can change while auto exposure settles. Saved front-camera images are unmirrored regardless of the preview preference.

On an error, re-enable VitaShell USB and inspect only `data/VitaCamera/session.log`, `settings.ini` and the relevant photo/JSON pair. Do not copy unrelated device files.
