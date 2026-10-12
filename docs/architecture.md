# Architecture

Vita Camera separates camera acquisition, live rendering and file export. Platform-independent C++ handles image processing, preferences, gallery indexing and sharing; Vita-specific code owns device APIs and GPU resources.

## Modules

| Source | Responsibility |
| --- | --- |
| `main.cpp` | Input, screen transitions, timers, capture scheduling and recovery |
| `camera.cpp` | Camera lifecycle, CDRAM buffer, frame reads and camera settings |
| `core.cpp` | Preferences, exposure steps, color transforms, zoom and metadata |
| `preview.cpp` | Preview processing and graph analysis |
| `ui.cpp` | Shared layout and drawing through the `Painter` interface |
| `vita_painter.cpp`, `text_layout.cpp` | GPU rendering, textures, font caches and text placement |
| `capture_writer.cpp`, `export.cpp` | Background capture queue, PNG encoding and file promotion |
| `storage.cpp`, `gallery.cpp` | Vita storage, preferences and photo indexing |
| `sharing.cpp`, `sharing_page.cpp` | Local HTTP server, gallery pages and QR codes |

Source files live in [`src/`](../src/); public interfaces live in [`include/`](../include/). Host tests exercise the shared code without the Vita runtime.

## Camera and frame ownership

The camera writes 640 × 480 ABGR frames into a CDRAM buffer. Each fresh frame is copied into app-owned memory before another read can replace it. A submitted capture holds its own frame and settings snapshot, so later camera changes cannot alter a queued photograph.

Startup requests 30 fps, falling back to 60 fps if opening or starting fails. Device testing confirmed brighter indoor photographs after adopting 30 fps. The accepted mode is logged and included in metadata; actual shutter duration is not measured.

Settings use the SDK's enumerated values. Cycling skips values rejected with parameter or range errors, with attempts bounded by the mode table. Other failures stop the cycle. Successful setter readbacks are checked before updating the interface; numeric diagnostics go to `session.log`.

Capture waits for a fresh frame and a short settling interval after camera changes. This interval is a responsiveness measure, not an exposure lock or a calibrated bracketing delay. The application reopens the camera after suspension or repeated read failure.

## Rendering and analysis

The Vita renderer uses libvita2d with 4× MSAA. UI geometry is batched into triangle lists; text and images flush the batch to preserve drawing order. Gradients use vertex colors, and decorative wave coordinates are cached.

Preview processing combines color transforms, mirroring and warning overlays in one pass. ARM NEON processes eight pixels at a time; a scalar implementation produces the same output. Processing writes directly into alternating GPU textures, avoiding an intermediate upload copy. The older draw completes before its texture or geometry pool is reused.

Only the selected graph is sampled, at most every 100 ms. Analysis follows the visible crop, color look and preview orientation, before warning overlays. Hidden graphs stop updating. Highlights flag any RGB channel at 250 or above; shadows flag luminance at 5 or below.

Digital zoom uses GPU cropping in the preview. The export worker creates a bilinearly resampled copy while preserving the full-frame original. Guides, mirroring and warnings never enter the export path.

## Text and interaction

Source Sans 3 is rasterized into a separate libvita2d font cache for each pixel size because the underlying atlas keys omit size. Text placement uses cached FreeType ink bounds and advances to center visible labels. The system PGF font is the fallback. Preview and review textures use linear filtering.

Camera switching is touch-only. Clean view lasts while Circle or a viewfinder touch is held. `CleanViewHold` consumes Circle presses used to return from menus or cancel a timer until release; touch holds track the initiating contact. Screen transitions clear stale gesture targets, and resume resets hold tracking.

Host SVG previews share the layout and shape geometry, but use browser font rasterization. Native antialiasing and touch behavior require device checks.

## Saving and browsing

The capture worker accepts one outstanding job, bounding memory use. PNGs are written to `.part` files, closed, then renamed. Existing capture names are rejected. The original is saved first; optional color or zoom copies and JSON metadata follow.

A failed capture stays in memory for retry while the application remains open. If an earlier attempt already saved the original, retry uses a new suffix to preserve that file. The worker joins during shutdown.

Gallery indexing groups edited copies with their originals and sorts captures newest first. Only the selected image is decoded. An unreadable image shows an unavailable state without preventing further browsing.

## Local sharing

The HTTP worker runs only while Share is open; the camera pauses during this screen. The server binds to the connected Wi-Fi address and serves completed gallery PNGs through a random 128-bit session path. Only GET and HEAD are accepted. Request-size limits, nonblocking sockets and timeouts keep shutdown responsive.

Leaving Share, losing the connection or resuming from suspension ends the session. Gallery pages are paginated and script-free. Images open inline for iPhone's save-to-Photos action, with separate attachment downloads. The same server runs on host loopback for integration tests.

## Build safeguards

The Vita executable links the complete pthread archive. libstdc++ references some thread functions weakly; ordinary static linking can omit them while still producing a VPK. [`check-vita-link.cmake`](../scripts/check-vita-link.cmake) verifies the linked executable before packaging.

Release builds disable profiling. Optional timing instrumentation and its limits are described in [Performance](performance.md). See [Device testing](device-testing.md) for behavior that still requires physical validation.
