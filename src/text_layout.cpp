#include "text_layout.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <vector>

namespace cam {
struct TextLayout::Impl {
    struct Glyph {
        int left, top, width, height, advance;
    };
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    std::vector<unsigned char> bytes;
    std::map<std::pair<unsigned, uint32_t>, Glyph> glyphs;
    ~Impl() {
        if (face)
            FT_Done_Face(face);
        if (library)
            FT_Done_FreeType(library);
    }
};
TextLayout::TextLayout() : impl_(std::make_unique<Impl>()) { FT_Init_FreeType(&impl_->library); }
TextLayout::~TextLayout() = default;
bool TextLayout::load(const void* data, size_t size) {
    if (impl_->face)
        FT_Done_Face(impl_->face);
    impl_->face = nullptr;
    impl_->glyphs.clear();
    impl_->bytes.clear();
    if (!impl_->library || !data || !size)
        return false;
    const auto* bytes = static_cast<const unsigned char*>(data);
    impl_->bytes.assign(bytes, bytes + size);
    return FT_New_Memory_Face(impl_->library, impl_->bytes.data(), impl_->bytes.size(), 0, &impl_->face) == 0;
}
bool TextLayout::loadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    const std::vector<char> bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    return load(bytes.data(), bytes.size());
}
TextBounds TextLayout::bounds(unsigned size, const std::string& text) {
    if (!impl_->face || !size)
        return {};
    int pen = 0, baseline = 0, left = 0, right = 0, top = 0, bottom = 0;
    bool ink = false;
    for (size_t i = 0; i < text.size();) {
        uint32_t code = static_cast<unsigned char>(text[i++]);
        if (code >= 128) {
            const int length = code < 0xe0 ? 1 : code < 0xf0 ? 2 : 3;
            code &= (1u << (6 - length)) - 1;
            for (int n = 0; n < length; ++n) {
                if (i >= text.size() || (static_cast<unsigned char>(text[i]) & 0xc0) != 0x80) {
                    code = 0xfffd;
                    break;
                }
                code = (code << 6) | (static_cast<unsigned char>(text[i++]) & 0x3f);
            }
        }
        if (code == '\n') {
            pen = 0;
            baseline += int(size);
            continue;
        }
        const auto key = std::make_pair(size, code);
        auto found = impl_->glyphs.find(key);
        if (found == impl_->glyphs.end()) {
            if (FT_Set_Pixel_Sizes(impl_->face, 0, size))
                return {};
            if (FT_Load_Char(impl_->face, code, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL))
                continue;
            const auto* g = impl_->face->glyph;
            found = impl_->glyphs
                        .emplace(key, Impl::Glyph{g->bitmap_left, g->bitmap_top, int(g->bitmap.width),
                                                  int(g->bitmap.rows), int(g->advance.x >> 6)})
                        .first;
        }
        const auto& g = found->second;
        if (g.width && g.height) {
            const int x = pen + g.left, y = baseline - g.top;
            if (!ink) {
                left = x;
                top = y;
                right = x + g.width;
                bottom = y + g.height;
                ink = true;
            } else {
                left = std::min(left, x);
                top = std::min(top, y);
                right = std::max(right, x + g.width);
                bottom = std::max(bottom, y + g.height);
            }
        }
        pen += g.advance;
    }
    return {float(left), float(top), float(right - left), float(bottom - top)};
}
}
