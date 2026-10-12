# Architecture

`core.cpp` owns validated preferences, exposure steps, image analysis, color transforms and metadata. Camera settings, app processing and PNG encoding remain separate.

`camera.cpp` owns a CDRAM capture buffer and the open/start/read/stop lifecycle. Each successful new frame is copied into app memory before another read. Camera setters are checked; rejected changes retain the preceding value. Getter results and errors go to `ux0:data/VitaCamera/session.log`.

`main.cpp` handles controller/touch input, timer, last-shot review and camera recovery after resume or repeated read failure. Capture waits for a fresh frame and a short settling interval after camera changes. That interval is a usability precaution, not a measured exposure lock or a calibrated bracketing delay.

`capture_writer.cpp` owns an immutable frame copy and encodes on a separate thread. One outstanding capture bounds memory use. A failed job stays in memory for explicit retry; closing the process loses that retained copy. PNGs are written to `.part` files, closed, then renamed. Existing capture names are rejected. If a partial export already saved an original, retry uses a new suffix and preserves it. Host tests run the same worker and exercise ownership, retry and shutdown.

The Vita link retains the complete pthread archive: libstdc++ references some thread functions weakly, so ordinary static linking can omit them while still producing a VPK. `scripts/check-vita-link.cmake` checks the linked ARM executable before packaging.

`ui.cpp` renders through a small `Painter` interface. `vita_painter.cpp` uses libvita2d with 4× MSAA. Rounded rectangles, rings and lines use filled triangle geometry, avoiding scanline corners and aliased hardware lines. The GPU finishes before frame vertex memory is reused or preview textures are updated. Mirroring, zebra stripes, shadow warnings and guides affect preview only.

Source Sans 3 is bundled with its OFL license. FreeType rasterizes each requested pixel size into a separate libvita2d font cache: its atlas keys omit size, so sharing one cache would reuse scaled glyphs. The system PGF font remains a fallback if the bundled font cannot load. Loaded review thumbnails use linear filtering, as live previews do.

The host SVG renderer shares layout and curved-shape semantics, but uses browser text rasterization. It is a layout check, not evidence of native antialiasing. On-device PS + START screenshots are the visual reference.

The Original look adds no color processing. Mono, Warm and Cool are explicit app transforms, with deterministic pixel tests. They are not claims about stock-camera color matching. Analysis measures the displayed image before warning overlays; zebras flag any RGB channel at 250+, shadow warnings flag luma at 5 or below.

## References

- [VitaSDK camera definitions](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/camera.h)
- [VitaSDK camera sample](https://github.com/vitasdk/samples/blob/master/camera/src/main.c)
- [libvita2d](https://github.com/xerpi/libvita2d)

VitaSDK exposes WB IDs 0, 1, 2 and 4; cycling contiguous integers would select an undefined mode. EV follows the SDK's enumerated steps. Export metadata labels controls as requested modes, not measured exposure values.
