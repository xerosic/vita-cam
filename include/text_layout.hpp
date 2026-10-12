#pragma once
#include <cstddef>
#include <memory>
#include <string>

namespace cam {
struct TextBounds { float x = 0, y = 0, width = 0, height = 0; };
class TextLayout {
public:
    TextLayout();
    ~TextLayout();
    bool load(const void* data, size_t size);
    bool loadFile(const std::string& path);
    TextBounds bounds(unsigned size, const std::string& text);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
