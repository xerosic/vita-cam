# Architecture

`core.cpp` owns validated preferences, exposure steps, image analysis, color transforms and metadata. Camera settings, app processing and PNG encoding remain separate.

`camera.cpp` owns a CDRAM capture buffer and the open/start/read/stop lifecycle. Each successful new frame is copied into app memory before another read. Camera setters are checked; rejected changes retain the preceding value. Getter results and errors go to `ux0:data/VitaCamera/session.log`.

`main.cpp` handles controller/touch input, timer, last-shot review and camera recovery after resume or repeated read failure. Capture waits for a fresh frame and a short settling interval after camera changes. That interval is a usability precaution, not a measured exposure lock or a calibrated bracketing delay.

`capture_writer.cpp` owns an immutable frame copy and encodes on a separate thread. One outstanding capture bounds memory use. A failed job stays in memory for explicit retry; closing the process loses that retained copy. PNGs are written to `.part` files, closed, then renamed. Existing capture names are rejected. If a partial export already saved an original, retry uses a new suffix and preserves it. Host tests run the same worker and exercise ownership, retry and shutdown.

`ui.cpp` renders through a small `Painter` interface. `vita_painter.cpp` uses libvita2d and the system PGF font. The host snapshot renderer uses the same layout code. Preview textures are updated only after the previous GPU work completes. Mirroring, zebra stripes, shadow warnings and guides affect preview only.

The Original look adds no color processing. Mono, Warm and Cool are explicit app transforms, with deterministic pixel tests. They are not claims about stock-camera color matching. Analysis measures the displayed image before warning overlays; zebras flag any RGB channel at 250+, shadow warnings flag luma at 5 or below.

## References

- [VitaSDK camera definitions](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/camera.h)
- [VitaSDK camera sample](https://github.com/vitasdk/samples/blob/master/camera/src/main.c)
- [libvita2d](https://github.com/xerpi/libvita2d)

VitaSDK exposes WB IDs 0, 1, 2 and 4; cycling contiguous integers would select an undefined mode. EV follows the SDK's enumerated steps. Export metadata labels controls as requested modes, not measured exposure values.
