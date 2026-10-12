#pragma once
#include "core.hpp"

namespace cam {
struct PreviewOptions {
    Look look = Look::Original;
    bool mirror = false;
    bool highlights = false;
    bool shadows = false;
};
bool renderPreview(const std::vector<Pixel>& source, Pixel* target, size_t stride, const PreviewOptions& options);
Analysis analyzePreview(const std::vector<Pixel>& source, const PreviewOptions& options, float zoom, Graph graph);
}
