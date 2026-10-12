# Performance

Live preview combines color processing, mirroring and warning overlays in one pass, writing directly into alternating GPU textures. ARM NEON handles eight pixels per iteration. Batched UI geometry and graph sampling at 10 Hz reduce work around the preview.

Host benchmarks measure processing cost. Device logs measure the complete application loop, including camera and display waits. The results below describe those separately.

## Device measurements

These measurements were collected with version 01.04, the Original look and a **60 fps camera request**. Each sample covers five seconds of continuous preview.

| Graph | Warnings | Samples | Median UI / fresh-frame rate | Sample range |
| --- | --- | ---: | ---: | ---: |
| Off | Highlights | 3 | 31.5 fps | 30.6–31.6 fps |
| Off | Both | 6 | 29.9 fps | 27.1–29.9 fps |
| Waveform | Highlights | 5 | 28.3 fps | 26.6–28.5 fps |

Device testing confirmed smooth preview after the 01.04 rendering changes. No timing baseline was recorded for the previous release, so these results do not establish a device speedup ratio.

One sample spanning 46.8 seconds away from the camera during sharing was excluded. Since 01.05, reopening the camera resets the profiler window, including after sharing or suspension.

Version 01.08 prefers **30 fps** and produced brighter indoor photographs in device testing. The table above predates that change; current timing, noise and motion-blur comparisons remain unmeasured.

## Host benchmark

Build and run the preview benchmark from the repository root:

```sh
cmake -S . -B build/benchmark -DVITACAM_HOST=ON \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE='-O2 -DNDEBUG'
cmake --build build/benchmark --target preview_benchmark --parallel
build/benchmark/preview_benchmark
```

Reference run: macOS ARM64, AppleClang 21, 2026-10-12. VGA input combines black, white and deterministic random pixels. Each result is the median of five batches of 60 frames. The benchmark requires byte-for-byte agreement between the previous and fused implementations before timing them.

| Look | Warnings | Previous CPU path | Fused CPU path |
| --- | --- | ---: | ---: |
| Original | Off | 0.636 ms | 0.060 ms |
| Original | Highlights | 0.987 ms | 0.113 ms |
| Original | Shadows | 1.244 ms | 0.114 ms |
| Original | Both | 1.134 ms | 0.114 ms |
| Mono | Both | 1.127 ms | 0.118 ms |
| Warm | Both | 1.140 ms | 0.149 ms |
| Cool | Both | 0.905 ms | 0.149 ms |

These timings exclude camera acquisition, Vita memory behavior, graphs, GPU work and display synchronization. They cannot predict Vita frame rates.

## Vita timings

Enable the optional profiler:

```sh
sh scripts/build.sh -DVITACAM_PROFILE=ON
```

The application writes five-second samples to `ux0:data/VitaCamera/session.log`. Changing the graph, look, warnings or view starts a new measurement window. Use the same scene, allow at least ten seconds per combination, and avoid taking photographs during steady-state measurements.

| Field | Meaning |
| --- | --- |
| `ui_fps` | Application loop iterations per second |
| `camera_fps` | Fresh camera frames received per second |
| `max_interval_ms` | Longest loop interval in the sample |
| `read` | Mean camera read and frame-copy time |
| `preview` | Mean preview-processing time; may include capture submission |
| `analysis` | Mean graph-analysis time |
| `other` | Mean housekeeping time |
| `draw_wait` | Mean rendering and GPU/display synchronization time |

Mean times are in microseconds per UI iteration. The frame rate stored in photo metadata is the requested camera mode, not a measurement.

Compare warnings Off, Highlights, Shadows and both, then repeat with RGB, Waveform and a color look. Save the log before relaunching; `session.log` is replaced at startup. See [Device testing](device-testing.md#collecting-diagnostics) for retrieval.

Run `sh scripts/build.sh` to restore the normal build with profiling disabled.
