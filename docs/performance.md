# Performance

Version 01.04 replaces separate look, overlay and upload passes with a single preview pass. NEON handles eight pixels at a time and writes directly to alternating GPU textures. UI geometry is batched; gradients use two triangles. Only the visible graph is sampled, at 10 Hz. Exports retain the existing capture pipeline.

## Host benchmark

```sh
cmake -S . -B build/benchmark -DVITACAM_HOST=ON \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE='-O2 -DNDEBUG'
cmake --build build/benchmark --target preview_benchmark --parallel
build/benchmark/preview_benchmark
```

Measured on macOS ARM64 with AppleClang 21, 2026-10-12. VGA input mixes black, white and deterministic random pixels. Results are the median of five batches of 60 frames; old and new output must match byte for byte before timing.

| Look | Warnings | Previous CPU path | Fused CPU path |
| --- | --- | ---: | ---: |
| Original | Off | 0.636 ms | 0.060 ms |
| Original | Highlights | 0.987 ms | 0.113 ms |
| Original | Shadows | 1.244 ms | 0.114 ms |
| Original | Both | 1.134 ms | 0.114 ms |
| Mono | Both | 1.127 ms | 0.118 ms |
| Warm | Both | 1.140 ms | 0.149 ms |
| Cool | Both | 0.905 ms | 0.149 ms |

These are host CPU timings, **not Vita frame rates**. They exclude camera acquisition, CDRAM behavior, graphs, GPU work and display synchronization.

## Device results

The user confirmed smooth preview and working sharing on 01.04. The retrieved device log records these five-second samples with the Original look:

| Graph | Warnings | Samples | Median UI / fresh-camera rate | Sample range |
| --- | --- | ---: | ---: | ---: |
| Off | Highlights | 3 | 31.5 fps | 30.6–31.6 fps |
| Off | Both | 6 | 29.9 fps | 27.1–29.9 fps |
| Waveform | Highlights | 5 | 28.3 fps | 26.6–28.5 fps |

One additional sample includes 46.8 seconds spent away from the camera during sharing and is excluded: it is not a continuous preview measurement. In 01.05, reopening the camera resets the profiler window, including after sharing or suspension. The retained log is `build/device-logs/01.04-session.log` (ignored). No 01.03 timing baseline was recorded, so these numbers do not establish a measured device speedup ratio.

## Vita timings

```sh
sh scripts/build.sh -DVITACAM_PROFILE=ON
```

This build writes five-second timing samples to `ux0:data/VitaCamera/session.log`. No diagnostic UI is added. Each setting combination starts a new window. Compare the same scene with warnings Off, Highlights, Shadows and both; allow ten seconds per combination. Repeat with RGB, Waveform and a color look.

Each `perf` line records UI loop rate, fresh camera frames per second, longest loop interval and mean time per UI iteration for camera read/copy, preview processing, analysis, housekeeping, and draw/synchronization. Preview time can include capture submission; avoid taking pictures during steady-state comparisons. `draw_wait` includes GPU/display waits, not just CPU drawing time. The FPS recorded in photo metadata is the requested camera mode, not a measurement.

Run `sh scripts/build.sh` for the normal build with profiling disabled. Save logs before relaunching: `session.log` is reset at startup.
