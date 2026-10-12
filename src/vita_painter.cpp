#include "vita_painter.hpp"
#include "camera.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>

namespace cam {
VitaPainter::VitaPainter() {
    logResult("startup: graphics",0);
    if (!vita2d_init_advanced_with_msaa(1024*1024,SCE_GXM_MULTISAMPLE_4X)) return;
    vita2d_set_clear_color(rgba(2,10,24));
    logResult("startup: font",0);
    std::ifstream file("app0:assets/fonts/SourceSans3-Regular.otf",std::ios::binary);
    fontData_.assign(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
    if (fontData_.size() < 4 || std::memcmp(fontData_.data(),"OTTO",4) != 0) fontData_.clear();
    if (fontData_.empty() || !font(18)) {
        logResult("startup: fallback font",-1);
        fallbackFont_ = vita2d_load_default_pgf();
    }
    logResult("startup: preview textures",0);
    live_ = vita2d_create_empty_texture(Width,Height);
    review_ = vita2d_create_empty_texture(Width,Height);
    for (auto* texture : {live_,review_}) if (texture) {
        vita2d_texture_set_filters(texture,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);
        std::memset(vita2d_texture_get_datap(texture),0,vita2d_texture_get_stride(texture)*Height);
    }
    logResult("startup: renderer ready",valid() ? 0 : -1);
}
VitaPainter::~VitaPainter() {
    vita2d_wait_rendering_done();
    if (live_) vita2d_free_texture(live_);
    if (review_) vita2d_free_texture(review_);
    for (auto& entry : fonts_) if (entry.second) vita2d_free_font(entry.second);
    if (fallbackFont_) vita2d_free_pgf(fallbackFont_);
    vita2d_fini();
}
bool VitaPainter::valid() const { return ((!fonts_.empty() && fonts_.begin()->second) || fallbackFont_) && live_ && review_; }
vita2d_font* VitaPainter::font(unsigned size) {
    auto found = fonts_.find(size);
    if (found != fonts_.end()) return found->second;
    if (fontData_.empty()) return nullptr;
    // libvita2d's atlas keys omit size, so each pixel size needs its own cache.
    auto* result = vita2d_load_font_mem(fontData_.data(),fontData_.size());
    fonts_.emplace(size,result);
    logResult("font size",result ? 0 : -1,size);
    return result;
}
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
void VitaPainter::uploadReview(const Frame& frame, Look look, bool original, float zoom) {
    processFrame(frame.pixels,reviewPixels_,original ? Look::Original : look);
    if (!original && zoom > 1) { std::vector<Pixel> cropped; zoomFrame(reviewPixels_,cropped,zoom); reviewPixels_.swap(cropped); }
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
    vita2d_texture_set_filters(review_,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);
    return true;
}
void VitaPainter::begin() { vita2d_wait_rendering_done(); vita2d_start_drawing(); vita2d_clear_screen(); }
void VitaPainter::end() { vita2d_end_drawing(); vita2d_swap_buffers(); }
void VitaPainter::rect(Rect r, Color c) { vita2d_draw_rectangle(r.x,r.y,r.w,r.h,c); }
void VitaPainter::circle(float x,float y,float r,Color c) { vita2d_draw_fill_circle(x,y,r,c); }
namespace {
constexpr float Pi = 3.14159265f;
int arcSegments(float radius, float angle) {
    // Keep polygon deviation below one eighth of a display pixel.
    return std::max(1,int(std::ceil(angle/(2*std::acos(1-std::min(.125f/radius,1.f))))));
}
vita2d_color_vertex* vertices(unsigned count) {
    return static_cast<vita2d_color_vertex*>(vita2d_pool_memalign(count*sizeof(vita2d_color_vertex),sizeof(vita2d_color_vertex)));
}
}
void VitaPainter::round(Rect r,float radius,Color c) {
    if (r.w <= 0 || r.h <= 0) return;
    radius = std::clamp(radius,0.f,std::min(r.w,r.h)/2);
    if (radius == 0) { rect(r,c); return; }
    const int segments = arcSegments(radius,Pi/2);
    const unsigned count = 4*(segments+1)+2;
    auto* v = vertices(count);
    if (!v) return;
    v[0] = {r.x+r.w/2,r.y+r.h/2,.5f,c};
    unsigned index = 1;
    for (int corner = 0; corner < 4; ++corner) {
        const float cx = corner < 2 ? r.x+r.w-radius : r.x+radius;
        const float cy = corner == 0 || corner == 3 ? r.y+radius : r.y+r.h-radius;
        for (int i = 0; i <= segments; ++i) {
            const float angle = (corner-1+i/float(segments))*Pi/2;
            v[index++] = {cx+radius*std::cos(angle),cy+radius*std::sin(angle),.5f,c};
        }
    }
    v[index] = v[1];
    vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLE_FAN,v,count);
}
void VitaPainter::ring(float x,float y,float radius,Color c) {
    if (radius <= .5f) { circle(x,y,std::max(radius,0.f),c); return; }
    const int segments = arcSegments(radius+.5f,2*Pi);
    const unsigned count = 2*(segments+1);
    auto* v = vertices(count);
    if (!v) return;
    for (int i = 0; i <= segments; ++i) {
        const float angle = (i%segments)*2*Pi/segments;
        const float dx = std::cos(angle), dy = std::sin(angle);
        v[2*i] = {x+dx*(radius+.5f),y+dy*(radius+.5f),.5f,c};
        v[2*i+1] = {x+dx*(radius-.5f),y+dy*(radius-.5f),.5f,c};
    }
    vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLE_STRIP,v,count);
}
void VitaPainter::line(float x,float y,float x2,float y2,Color c) {
    const float length = std::hypot(x2-x,y2-y);
    if (length == 0) return;
    const float dx = -(y2-y)*.5f/length, dy = (x2-x)*.5f/length;
    auto* v = vertices(4);
    if (!v) return;
    x += .5f; y += .5f; x2 += .5f; y2 += .5f;
    v[0] = {x+dx,y+dy,.5f,c}; v[1] = {x-dx,y-dy,.5f,c};
    v[2] = {x2+dx,y2+dy,.5f,c}; v[3] = {x2-dx,y2-dy,.5f,c};
    vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLE_STRIP,v,4);
}
void VitaPainter::text(float x,float y,float size,Color c,const std::string& t) {
    const unsigned pixels = unsigned(std::max(1.f,std::round(size)));
    if (auto* sizedFont = font(pixels)) vita2d_font_draw_text(sizedFont,int(std::round(x)),int(std::round(y)),c,pixels,t.c_str());
    else if (fallbackFont_) vita2d_pgf_draw_text(fallbackFont_,int(x),int(y),c,size/20.f,t.c_str());
}
void VitaPainter::image(int id,Rect dest,Rect src) {
    vita2d_draw_texture_part_scale(id == 0 ? live_ : review_,dest.x,dest.y,src.x,src.y,src.w,src.h,dest.w/src.w,dest.h/src.h);
}
}
