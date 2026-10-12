#pragma once
#include "ui.hpp"
#include <vita2d.h>
#include <map>

namespace cam {
class VitaPainter : public Painter {
public:
    VitaPainter();
    ~VitaPainter() override;
    bool valid() const;
    void uploadLive(const Frame& frame, const UiState& state);
    void uploadReview(const Frame& frame, Look look, bool original, float zoom = 1);
    bool loadReview(const std::string& path);
    void begin();
    void end();
    void rect(Rect r, Color color) override;
    void circle(float x, float y, float radius, Color color) override;
    void ring(float x, float y, float radius, Color color) override;
    void round(Rect r, float radius, Color color) override;
    void line(float x, float y, float x2, float y2, Color color) override;
    void text(float x, float baseline, float size, Color color, const std::string& text) override;
    void image(int id, Rect destination, Rect source) override;
    const std::vector<Pixel>& livePixels() const { return pixels_; }
private:
    void upload(vita2d_texture* texture, const std::vector<Pixel>& pixels);
    vita2d_font* font(unsigned size);
    std::vector<uint8_t> fontData_;
    std::map<unsigned,vita2d_font*> fonts_;
    vita2d_pgf* fallbackFont_ = nullptr;
    vita2d_texture* live_ = nullptr;
    vita2d_texture* review_ = nullptr;
    std::vector<Pixel> pixels_;
    std::vector<Pixel> reviewPixels_;
};
}
