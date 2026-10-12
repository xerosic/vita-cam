#include "vita_painter.hpp"
#include <algorithm>
#include <cstring>

namespace cam {
VitaPainter::VitaPainter() {
    if (!vita2d_init()) return;
    vita2d_set_clear_color(rgba(2,10,24));
    font_ = vita2d_load_default_pgf();
    live_ = vita2d_create_empty_texture(Width,Height);
    review_ = vita2d_create_empty_texture(Width,Height);
    for (auto* texture : {live_,review_}) if (texture) {
        vita2d_texture_set_filters(texture,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);
        std::memset(vita2d_texture_get_datap(texture),0,vita2d_texture_get_stride(texture)*Height);
    }
}
VitaPainter::~VitaPainter() {
    vita2d_wait_rendering_done();
    if (live_) vita2d_free_texture(live_);
    if (review_) vita2d_free_texture(review_);
    if (font_) vita2d_free_pgf(font_);
    vita2d_fini();
}
bool VitaPainter::valid() const { return font_ && live_ && review_; }
void VitaPainter::upload(vita2d_texture* texture, const std::vector<Pixel>& pixels) {
    vita2d_wait_rendering_done();
    auto* destination = static_cast<uint8_t*>(vita2d_texture_get_datap(texture));
    const unsigned stride = vita2d_texture_get_stride(texture);
    for (int y = 0; y < Height; ++y) std::memcpy(destination+y*stride,pixels.data()+y*Width,Width*sizeof(Pixel));
}
void VitaPainter::uploadLive(const Frame& f, const UiState& state) {
    processFrame(f.pixels,pixels_,state.compare ? Look::Original : state.settings.look,!state.settings.device && state.settings.mirror);
    if ((state.settings.zebras || state.settings.shadows) && !state.clean) {
        auto overlay = pixels_;
        for (int y = 0; y < Height; ++y) for (int x = 0; x < Width; ++x) {
            auto& p = overlay[y*Width+x];
            if (state.settings.zebras && std::max({p.r,p.g,p.b}) >= 250 && ((x+y)%12 < 4)) p = {255,195,83,255};
            if (state.settings.shadows && luminance(p) <= 5 && ((x+y)%12 < 4)) p = {72,142,255,255};
        }
        upload(live_,overlay);
    } else upload(live_,pixels_);
}
void VitaPainter::uploadReview(const Frame& frame, Look look, bool original) {
    processFrame(frame.pixels,reviewPixels_,original ? Look::Original : look);
    upload(review_,reviewPixels_);
}
bool VitaPainter::loadReview(const std::string& path) {
    vita2d_wait_rendering_done();
    auto* texture = vita2d_load_PNG_file(path.c_str());
    if (!texture) return false;
    if (vita2d_texture_get_width(texture) != Width || vita2d_texture_get_height(texture) != Height) {
        vita2d_free_texture(texture); return false;
    }
    vita2d_free_texture(review_); review_ = texture;
    return true;
}
void VitaPainter::begin() { vita2d_start_drawing(); vita2d_clear_screen(); }
void VitaPainter::end() { vita2d_end_drawing(); vita2d_swap_buffers(); }
void VitaPainter::rect(Rect r, Color c) { vita2d_draw_rectangle(r.x,r.y,r.w,r.h,c); }
void VitaPainter::circle(float x,float y,float r,Color c) { vita2d_draw_fill_circle(x,y,r,c); }
void VitaPainter::line(float x,float y,float x2,float y2,Color c) { vita2d_draw_line(x,y,x2,y2,c); }
void VitaPainter::text(float x,float y,float size,Color c,const std::string& t) { vita2d_pgf_draw_text(font_,int(x),int(y),c,size/20.f,t.c_str()); }
void VitaPainter::image(int id,Rect dest,Rect src) {
    vita2d_draw_texture_part_scale(id == 0 ? live_ : review_,dest.x,dest.y,src.x,src.y,src.w,src.h,dest.w/src.w,dest.h/src.h);
}
}
