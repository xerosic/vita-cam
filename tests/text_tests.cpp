#include "ui.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <cassert>
#include <cmath>

using namespace cam;
class RasterPainter : public Painter {
public:
    TextLayout layout;
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    std::array<unsigned char, 320 * 160> pixels{};
    explicit RasterPainter(const char* path) {
        assert(layout.loadFile(path));
        assert(FT_Init_FreeType(&library) == 0);
        assert(FT_New_Face(library, path, 0, &face) == 0);
    }
    ~RasterPainter() {
        FT_Done_Face(face);
        FT_Done_FreeType(library);
    }
    void rect(Rect, Color) override {}
    void circle(float, float, float, Color) override {}
    void ring(float, float, float, Color) override {}
    void round(Rect, float, Color) override {}
    void line(float, float, float, float, Color) override {}
    void image(int, Rect, Rect) override {}
    TextBounds textBounds(float size, const std::string& value) override {
        return layout.bounds(unsigned(size), value);
    }
    void text(float x, float y, float size, Color, const std::string& value) override {
        assert(FT_Set_Pixel_Sizes(face, 0, unsigned(size)) == 0);
        int pen = int(std::round(x));
        for (unsigned char c : value) {
            assert(FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL) == 0);
            const auto* glyph = face->glyph;
            for (unsigned row = 0; row < glyph->bitmap.rows; ++row)
                for (unsigned col = 0; col < glyph->bitmap.width; ++col) {
                    const int px = pen + glyph->bitmap_left + int(col),
                              py = int(std::round(y)) - glyph->bitmap_top + int(row);
                    assert(px >= 0 && px < 320 && py >= 0 && py < 160);
                    pixels[py * 320 + px] |= glyph->bitmap.buffer[row * glyph->bitmap.pitch + col];
                }
            pen += glyph->advance.x >> 6;
        }
    }
};
int main(int argc, char** argv) {
    assert(argc == 2);
    RasterPainter painter(argv[1]);
    const Rect button{20, 30, 280, 96};
    for (unsigned size : {18, 36, 13, 58, 18})
        for (const char* label : {"+", "-", "Capture", "Settings", "Shooting", "Retry save", "10"}) {
            painter.pixels.fill(0);
            painter.centeredText(button, float(size), 0, label);
            int left = 320, top = 160, right = 0, bottom = 0;
            for (int y = 0; y < 160; ++y)
                for (int x = 0; x < 320; ++x)
                    if (painter.pixels[y * 320 + x]) {
                        left = std::min(left, x);
                        right = std::max(right, x + 1);
                        top = std::min(top, y);
                        bottom = std::max(bottom, y + 1);
                    }
            assert(right > left && bottom > top);
            assert(std::abs((left + right) / 2.f - (button.x + button.w / 2)) <= 1);
            assert(std::abs((top + bottom) / 2.f - (button.y + button.h / 2)) <= 1);
        }
    assert(painter.layout.bounds(18, "").width == 0);
    assert(painter.layout.bounds(18, "   ").height == 0);
    assert(painter.layout.bounds(0, "Capture").width == 0);
    assert(!painter.layout.load(nullptr, 0));
    assert(painter.layout.bounds(18, "Capture").width == 0);
    assert(painter.layout.loadFile(argv[1]));
    assert(painter.layout.bounds(18, "Capture").width > 0);
}
