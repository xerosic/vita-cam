#include "preview.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>

int main() {
    using namespace cam;
    std::vector<Pixel> source(Width*Height), original;
    uint32_t random = 12345;
    for (size_t i = 0; i < source.size(); ++i) {
        random ^= random<<13; random ^= random>>17; random ^= random<<5;
        source[i] = i%5 == 0 ? Pixel{0,0,0,0} : i%5 == 1 ? Pixel{255,255,255,0} : Pixel{uint8_t(random),uint8_t(random>>8),uint8_t(random>>16),uint8_t(random>>24)};
    }
    for (unsigned i = 0; i < 256; ++i) {
        source[i] = {uint8_t(i),uint8_t(i),uint8_t(i),0};
        source[Width+i] = {uint8_t(i),0,0,0};
        source[2*Width+i] = {0,uint8_t(i),0,0};
        source[3*Width+i] = {0,0,uint8_t(i),0};
    }
    original = source;
    const size_t stride = Width+8;
    const Pixel guard{17,31,47,63};
    std::vector<Pixel> result(stride*Height,guard), reference;
    for (Look look : {Look::Original,Look::Mono,Look::Warm,Look::Cool}) for (bool mirror : {false,true}) for (unsigned flags = 0; flags < 4; ++flags) {
        const PreviewOptions options{look,mirror,bool(flags&1),bool(flags&2)};
        processFrame(source,reference,look,mirror);
        for (int y = 0; y < Height; ++y) for (int x = 0; x < Width; ++x) {
            auto& p = reference[y*Width+x];
            if (options.highlights && std::max({p.r,p.g,p.b}) >= 250 && (x+y)%12 < 4) p = {255,195,83,255};
            if (options.shadows && luminance(p) <= 5 && (x+y)%12 < 4) p = {72,142,255,255};
        }
        assert(renderPreview(source,result.data(),stride,options));
        for (int y = 0; y < Height; ++y) {
            assert(std::memcmp(result.data()+y*stride,reference.data()+y*Width,Width*sizeof(Pixel)) == 0);
            for (size_t x = Width; x < stride; ++x) assert(std::memcmp(&result[y*stride+x],&guard,sizeof(Pixel)) == 0);
        }
        assert(std::memcmp(source.data(),original.data(),source.size()*sizeof(Pixel)) == 0);
    }
    assert(!renderPreview({},result.data(),stride,{}));
    assert(!renderPreview(source,nullptr,stride,{}));
    assert(!renderPreview(source,result.data(),Width-1,{}));
    for (float zoom : {1.f,2.f,4.f}) for (bool mirror : {false,true}) {
        processFrame(source,reference,Look::Warm,mirror);
        const auto expected = analyze(reference,zoom);
        for (Graph graph : {Graph::Luminance,Graph::Rgb,Graph::Waveform}) {
            const auto actual = analyzePreview(source,{Look::Warm,mirror,true,true},zoom,graph);
            if (graph == Graph::Luminance) assert(actual.bins == expected.bins && actual.peak == expected.peak);
            if (graph == Graph::Rgb) assert(actual.rgb == expected.rgb && actual.rgbPeak == expected.rgbPeak);
            if (graph == Graph::Waveform) assert(actual.waveform == expected.waveform && actual.waveformPeak == expected.waveformPeak);
            assert(actual.highlights == expected.highlights && actual.shadows == expected.shadows);
        }
    }
    assert(analyzePreview(source,{},1,Graph::Off).peak == 0);
}
