#include "preview.hpp"
#include <algorithm>
#if defined(__ARM_NEON) && !defined(VITACAM_SCALAR_PREVIEW)
#include <arm_neon.h>
#endif

namespace cam {
namespace {
constexpr auto stripeMasks() {
    std::array<std::array<uint8_t, 8>, 12> masks{};
    for (size_t phase = 0; phase < masks.size(); ++phase)
        for (size_t x = 0; x < 8; ++x)
            masks[phase][x] = (phase + x) % 12 < 4 ? 255 : 0;
    return masks;
}
constexpr auto Stripes = stripeMasks();

#if defined(__ARM_NEON) && !defined(VITACAM_SCALAR_PREVIEW)
uint8x8_t luma(uint8x8x4_t p) {
    auto sum = vmull_u8(p.val[0], vdup_n_u8(77));
    sum = vmlal_u8(sum, p.val[1], vdup_n_u8(150));
    return vshrn_n_u16(vmlal_u8(sum, p.val[2], vdup_n_u8(29)), 8);
}
uint8x8_t scaleChannel(uint8x8_t channel, unsigned percent, unsigned offset = 0) {
    const auto product = vmull_u8(channel, vdup_n_u8(percent));
    // Exact division by 100 for the channel products (at most 255 * 106).
    constexpr unsigned Shift = 19, Reciprocal = ((1u << Shift) + 99) / 100;
    const auto low = vmovn_u32(vshrq_n_u32(vmull_n_u16(vget_low_u16(product), Reciprocal), Shift));
    const auto high = vmovn_u32(vshrq_n_u32(vmull_n_u16(vget_high_u16(product), Reciprocal), Shift));
    return vqmovn_u16(vaddq_u16(vcombine_u16(low, high), vdupq_n_u16(offset)));
}
#endif

template <Look look, bool warnings>
void render(const Pixel* source, Pixel* target, size_t stride, const PreviewOptions& options) {
    for (int y = 0; y < Height; ++y) {
        const Pixel* row = source + y * Width;
        Pixel* destination = target + y * stride;
        int phase = y % 12;
        for (int x = 0; x < Width; x += 8) {
#if defined(__ARM_NEON) && !defined(VITACAM_SCALAR_PREVIEW)
            auto p = vld4_u8(reinterpret_cast<const uint8_t*>(row + (options.mirror ? Width - x - 8 : x)));
            if (options.mirror)
                for (int c = 0; c < 3; ++c)
                    p.val[c] = vrev64_u8(p.val[c]);
            if constexpr (look == Look::Mono)
                p.val[0] = p.val[1] = p.val[2] = luma(p);
            if constexpr (look == Look::Warm) {
                p.val[0] = scaleChannel(p.val[0], 106, 3);
                p.val[2] = scaleChannel(p.val[2], 92);
            }
            if constexpr (look == Look::Cool) {
                p.val[0] = scaleChannel(p.val[0], 94);
                p.val[2] = scaleChannel(p.val[2], 105, 3);
            }
            if constexpr (warnings) {
                const auto stripe = vld1_u8(Stripes[phase].data());
                const auto high =
                    options.highlights
                        ? vand_u8(stripe, vcge_u8(vmax_u8(vmax_u8(p.val[0], p.val[1]), p.val[2]), vdup_n_u8(250)))
                        : vdup_n_u8(0);
                const auto low = options.shadows ? vand_u8(stripe, vcle_u8(luma(p), vdup_n_u8(5))) : vdup_n_u8(0);
                p.val[0] = vbsl_u8(low, vdup_n_u8(72), vbsl_u8(high, vdup_n_u8(255), p.val[0]));
                p.val[1] = vbsl_u8(low, vdup_n_u8(142), vbsl_u8(high, vdup_n_u8(195), p.val[1]));
                p.val[2] = vbsl_u8(low, vdup_n_u8(255), vbsl_u8(high, vdup_n_u8(83), p.val[2]));
            }
            p.val[3] = vdup_n_u8(255);
            vst4_u8(reinterpret_cast<uint8_t*>(destination + x), p);
#else
            for (int lane = 0; lane < 8; ++lane) {
                const Pixel input = row[options.mirror ? Width - 1 - x - lane : x + lane];
                Pixel p = input;
                const auto gray = [](Pixel c) { return uint8_t((77u * c.r + 150u * c.g + 29u * c.b) >> 8); };
                if constexpr (look == Look::Mono)
                    p.r = p.g = p.b = gray(p);
                if constexpr (look == Look::Warm) {
                    p.r = uint8_t(std::min(255, p.r * 106 / 100 + 3));
                    p.b = uint8_t(p.b * 92 / 100);
                }
                if constexpr (look == Look::Cool) {
                    p.r = uint8_t(p.r * 94 / 100);
                    p.b = uint8_t(std::min(255, p.b * 105 / 100 + 3));
                }
                p.a = 255;
                if constexpr (warnings)
                    if (Stripes[phase][lane]) {
                        if (options.highlights && std::max({p.r, p.g, p.b}) >= 250)
                            p = {255, 195, 83, 255};
                        else if (options.shadows && gray(p) <= 5)
                            p = {72, 142, 255, 255};
                    }
                destination[x + lane] = p;
            }
#endif
            if constexpr (warnings) {
                phase += 8;
                if (phase >= 12)
                    phase -= 12;
            }
        }
    }
}
template <Look look> void dispatch(const Pixel* source, Pixel* target, size_t stride, const PreviewOptions& options) {
    if (options.highlights || options.shadows)
        render<look, true>(source, target, stride, options);
    else
        render<look, false>(source, target, stride, options);
}
}
bool renderPreview(const std::vector<Pixel>& source, Pixel* target, size_t stride, const PreviewOptions& options) {
    if (source.size() != size_t(Width * Height) || !target || stride < Width)
        return false;
    switch (options.look) {
    case Look::Original:
        dispatch<Look::Original>(source.data(), target, stride, options);
        break;
    case Look::Mono:
        dispatch<Look::Mono>(source.data(), target, stride, options);
        break;
    case Look::Warm:
        dispatch<Look::Warm>(source.data(), target, stride, options);
        break;
    case Look::Cool:
        dispatch<Look::Cool>(source.data(), target, stride, options);
        break;
    }
    return true;
}
Analysis analyzePreview(const std::vector<Pixel>& source, const PreviewOptions& options, float zoom, Graph graph) {
    Analysis result;
    if (graph == Graph::Off || source.size() != size_t(Width * Height))
        return result;
    const auto crop = zoomCrop(zoom);
    const float columnScale = 64 / crop.w;
    unsigned samples = 0, high = 0, low = 0;
    for (int y = int(crop.y); y < int(crop.y + crop.h); y += 4)
        for (int x = int(crop.x); x < int(crop.x + crop.w); x += 4) {
            const auto p = processPixel(source[y * Width + (options.mirror ? Width - 1 - x : x)], options.look);
            const uint8_t value = luminance(p);
            if (graph == Graph::Luminance)
                ++result.bins[value / 4];
            else if (graph == Graph::Rgb) {
                ++result.rgb[0][p.r / 4];
                ++result.rgb[1][p.g / 4];
                ++result.rgb[2][p.b / 4];
            } else
                ++result.waveform[(31 - value / 8) * 64 + std::clamp(int((x - crop.x) * columnScale), 0, 63)];
            ++samples;
            high += std::max({p.r, p.g, p.b}) >= 250;
            low += value <= 5;
        }
    result.peak = *std::max_element(result.bins.begin(), result.bins.end());
    for (const auto& channel : result.rgb)
        result.rgbPeak = std::max(result.rgbPeak, *std::max_element(channel.begin(), channel.end()));
    result.waveformPeak = *std::max_element(result.waveform.begin(), result.waveform.end());
    if (samples) {
        result.highlights = 100.f * high / samples;
        result.shadows = 100.f * low / samples;
    }
    return result;
}
}
