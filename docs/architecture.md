# Architecture

`core.cpp` owns validated preferences, exposure steps, image analysis, color transforms and metadata. Camera settings, app processing and PNG encoding remain separate.

`camera.cpp` owns a CDRAM capture buffer and the open/start/read/stop lifecycle. Each successful new frame is copied into app memory before another read. Camera setters are checked; rejected changes retain the preceding value. Getter results and errors go to `ux0:data/VitaCamera/session.log`.

Setting cycles skip values rejected with parameter/range errors, bounded by the mode table size; other failures stop the cycle. This avoids trapping the selector on unsupported modes without hard-coding device-specific exclusions. Successful setter readbacks are checked before displaying the requested value. UI errors use readable messages; numeric errors stay in the log.

`main.cpp` handles controller/touch input, timer, photo browsing and camera recovery after resume or repeated read failure. Capture waits for a fresh frame and a short settling interval after camera changes. That interval is a usability precaution, not a measured exposure lock or a calibrated bracketing delay.

`capture_writer.cpp` owns an immutable frame copy and encodes on a separate thread. One outstanding capture bounds memory use. A failed job stays in memory for explicit retry; closing the process loses that retained copy. PNGs are written to `.part` files, closed, then renamed. Existing capture names are rejected. If a partial export already saved an original, retry uses a new suffix and preserves it. Host tests run the same worker and exercise ownership, retry and shutdown.

The Vita link retains the complete pthread archive: libstdc++ references some thread functions weakly, so ordinary static linking can omit them while still producing a VPK. `scripts/check-vita-link.cmake` checks the linked ARM executable before packaging.

`ui.cpp` renders through a small `Painter` interface. `vita_painter.cpp` uses libvita2d with 4× MSAA. Rectangles, rounded corners, circles, rings and lines are batched into triangle lists; text and images flush the batch to preserve ordering. Gradients use vertex colors. Static decorative wave coordinates are computed once. The GPU finishes before the frame vertex pool is reused.

`preview.cpp` fuses the selected look, mirroring and warning overlays into one pass directly into an inactive GPU texture. ARM NEON handles eight pixels per iteration; a scalar fallback produces identical pixels. There is no per-frame overlay allocation or intermediate upload copy. Two textures alternate: the previous draw may use one while the CPU writes the other; `begin()` retires the older draw before its texture can be reused. Full-resolution preview continues in Settings but pauses behind Review and Help. Mirroring, warning stripes and guides never affect exports.

Source Sans 3 is bundled with its OFL license. FreeType rasterizes each requested pixel size into a separate libvita2d font cache: its atlas keys omit size, so sharing one cache would reuse scaled glyphs. `text_layout.cpp` caches glyph bounds and advances matching this renderer. Buttons center the visible text bounds rather than an estimated character width or baseline offset. The system PGF font remains a fallback if the bundled font cannot load. Loaded review thumbnails use linear filtering, as live previews do.

The host SVG renderer shares layout and curved-shape semantics, but uses browser text rasterization. It is a layout check, not evidence of native antialiasing. On-device PS + START screenshots are the visual reference.

The Original look adds no color processing. Mono, Warm and Cool are explicit app transforms, with deterministic pixel tests. They are not claims about stock-camera color matching. Analysis measures the displayed image before warning overlays; zebras flag any RGB channel at 250+, shadow warnings flag luma at 5 or below.

Graphs cycle between luminance, RGB histograms, waveform and Off. Only the selected graph is computed, at most every 100 ms, and analysis stops when hidden. Sampling reads the visible crop from the original frame, applying the selected look and preview orientation before counting pixels. Waveform density groups horizontal position and luminance. Legacy `histogram` preferences migrate to the graph mode. Right-stick zoom is time-based with a dead zone and spans 1–4×. Preview cropping runs on the GPU; the save worker creates a bilinearly resampled `-Zoom` copy, retaining the full-frame original. Zoom is recorded in JSON and does not claim additional sensor detail.

`gallery.cpp` scans completed PNGs in the app photo directory, groups color/zoom copies with their original, and sorts newest first by capture filename. Only the selected image is decoded. Failed reads show an unavailable state; browsing continues. D-pad and swipes browse, the right stick zooms, and the left stick pans. Returning to the camera restores the latest capture thumbnail.

`sharing.cpp` runs an HTTP worker only while Share is open. It binds the connected Wi-Fi address, uses a random 128-bit session path, and serves an allowlist of original/edited PNGs from the app gallery. It accepts GET/HEAD, caps request headers, and uses nonblocking sockets with timeouts so stopping can interrupt a slow client. The camera pauses during sharing. Leaving Share, a connection loss or returning from suspension ends the session. The same server runs on host loopback for integration tests.

`sharing_page.cpp` generates paginated, script-free HTML for phone browsers and a QR code through vendored Nayuki v1.8.0. Images open inline for iPhone's save-to-Photos menu; download links also provide PNG attachments. No cloud service, upload route or Bluetooth transfer is involved.

## References

- [VitaSDK camera definitions](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/camera.h)
- [VitaSDK camera sample](https://github.com/vitasdk/samples/blob/master/camera/src/main.c)
- [libvita2d](https://github.com/xerpi/libvita2d)

VitaSDK exposes WB IDs 0, 1, 2 and 4; cycling contiguous integers would select an undefined mode. EV follows the SDK's enumerated steps. Export metadata labels controls as requested modes, not measured exposure values.
