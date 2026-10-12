#include "preview.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>

using namespace cam;
namespace {
volatile unsigned sink = 0;
template <class Render> double measure(Render render, const std::vector<Pixel>& output) {
    std::array<double, 5> samples{};
    for (auto& sample : samples) {
        const auto start = std::chrono::steady_clock::now();
        for (unsigned frame = 0; frame < 60; ++frame) {
            render();
            sink = output[frame * 997 % output.size()].r;
        }
        sample = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / 60;
    }
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}
}
int main() {
    std::vector<Pixel> source(Width * Height), processed, previous(source.size()), fused(source.size());
    uint32_t random = 12345;
    for (size_t i = 0; i < source.size(); ++i) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        source[i] = i % 3 == 0   ? Pixel{0, 0, 0, 255}
                    : i % 3 == 1 ? Pixel{255, 255, 255, 255}
                                 : Pixel{uint8_t(random), uint8_t(random >> 8), uint8_t(random >> 16), 255};
    }
    std::puts("Host CPU only, 640x480; median of five 60-frame batches. Excludes camera, GPU and graphs.");
    std::puts("look       warnings    previous ms   fused ms   ratio");
    for (Look look : {Look::Original, Look::Mono, Look::Warm, Look::Cool})
        for (unsigned flags = 0; flags < 4; ++flags) {
            if (look != Look::Original && flags != 3)
                continue;
            const PreviewOptions options{look, false, bool(flags & 1), bool(flags & 2)};
            const auto oldPath = [&] {
                processFrame(source, processed, look);
                if (flags) {
                    auto overlay = processed;
                    for (int y = 0; y < Height; ++y)
                        for (int x = 0; x < Width; ++x) {
                            auto& p = overlay[y * Width + x];
                            if (options.highlights && std::max({p.r, p.g, p.b}) >= 250 && (x + y) % 12 < 4)
                                p = {255, 195, 83, 255};
                            if (options.shadows && luminance(p) <= 5 && (x + y) % 12 < 4)
                                p = {72, 142, 255, 255};
                        }
                    std::memcpy(previous.data(), overlay.data(), overlay.size() * sizeof(Pixel));
                } else
                    std::memcpy(previous.data(), processed.data(), processed.size() * sizeof(Pixel));
            };
            const auto newPath = [&] { renderPreview(source, fused.data(), Width, options); };
            oldPath();
            newPath();
            if (std::memcmp(previous.data(), fused.data(), fused.size() * sizeof(Pixel)))
                return 1;
            const double oldMs = measure(oldPath, previous), newMs = measure(newPath, fused);
            const char* labels[]{"off", "highlights", "shadows", "both"};
            std::printf("%-10s %-11s %10.3f %10.3f %7.2fx\n", LookNames[int(look)], labels[flags], oldMs, newMs,
                        oldMs / newMs);
        }
}
