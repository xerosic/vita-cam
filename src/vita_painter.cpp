#include "vita_painter.hpp"
#include "camera.hpp"
#include "preview.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>

namespace cam {
VitaPainter::VitaPainter() {
    geometry_.reserve(16380);
    logResult("startup: graphics",0);
    if (!vita2d_init_advanced_with_msaa(1024*1024,SCE_GXM_MULTISAMPLE_4X)) return;
    vita2d_set_clear_color(rgba(2,10,24));
    logResult("startup: font",0);
    std::ifstream file("app0:assets/fonts/SourceSans3-Regular.otf",std::ios::binary);
    fontData_.assign(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
    if (fontData_.size() < 4 || std::memcmp(fontData_.data(),"OTTO",4) != 0) fontData_.clear();
    textLayout_.load(fontData_.data(),fontData_.size());
    if (fontData_.empty() || !font(18)) {
        logResult("startup: fallback font",-1);
        fallbackFont_ = vita2d_load_default_pgf();
    }
    logResult("startup: preview textures",0);
    for (auto& texture : live_) texture = vita2d_create_empty_texture(Width,Height);
    review_ = vita2d_create_empty_texture(Width,Height);
    for (auto* texture : {live_[0],live_[1],review_}) if (texture) {
        vita2d_texture_set_filters(texture,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);
        std::memset(vita2d_texture_get_datap(texture),0,vita2d_texture_get_stride(texture)*Height);
    }
    logResult("startup: renderer ready",valid() ? 0 : -1);
}
VitaPainter::~VitaPainter() {
    vita2d_wait_rendering_done();
    for (auto* texture : live_) if (texture) vita2d_free_texture(texture);
    if (review_) vita2d_free_texture(review_);
    for (auto& entry : fonts_) if (entry.second) vita2d_free_font(entry.second);
    if (fallbackFont_) vita2d_free_pgf(fallbackFont_);
    vita2d_fini();
}
bool VitaPainter::valid() const { return ((!fonts_.empty() && fonts_.begin()->second) || fallbackFont_) && live_[0] && live_[1] && review_; }
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
    // begin() retired the older frame, so the texture not currently displayed is free.
    const unsigned next = liveIndex_^1;
    auto* texture = live_[next];
    const bool warnings = !state.clean && state.screen == Screen::Camera;
    const PreviewOptions options{state.compare ? Look::Original : state.settings.look,
        !state.settings.device && state.settings.mirror,warnings && state.settings.zebras,warnings && state.settings.shadows};
    if (renderPreview(f.pixels,static_cast<Pixel*>(vita2d_texture_get_datap(texture)),vita2d_texture_get_stride(texture)/sizeof(Pixel),options)) liveIndex_ = next;
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
void VitaPainter::end() { flush(); vita2d_end_drawing(); vita2d_swap_buffers(); }
void VitaPainter::rect(Rect r, Color c) {
    if (r.w <= 0 || r.h <= 0) return;
    triangle({r.x,r.y,.5f,c},{r.x+r.w,r.y,.5f,c},{r.x,r.y+r.h,.5f,c});
    triangle({r.x+r.w,r.y,.5f,c},{r.x+r.w,r.y+r.h,.5f,c},{r.x,r.y+r.h,.5f,c});
}
void VitaPainter::gradient(Rect r,Color top,Color bottom) {
    triangle({r.x,r.y,.5f,top},{r.x+r.w,r.y,.5f,top},{r.x,r.y+r.h,.5f,bottom});
    triangle({r.x+r.w,r.y,.5f,top},{r.x+r.w,r.y+r.h,.5f,bottom},{r.x,r.y+r.h,.5f,bottom});
}
namespace {
constexpr float Pi = 3.14159265f;
int arcSegments(float radius, float angle) {
    // Keep polygon deviation below one eighth of a display pixel.
    return std::max(1,int(std::ceil(angle/(2*std::acos(1-std::min(.125f/radius,1.f))))));
}
}
void VitaPainter::triangle(vita2d_color_vertex a,vita2d_color_vertex b,vita2d_color_vertex c) {
    if (geometry_.size() == 16380) flush();
    geometry_.push_back(a); geometry_.push_back(b); geometry_.push_back(c);
}
void VitaPainter::flush() {
    if (geometry_.empty()) return;
    const size_t bytes = geometry_.size()*sizeof(vita2d_color_vertex);
    auto* vertices = static_cast<vita2d_color_vertex*>(vita2d_pool_memalign(bytes,sizeof(vita2d_color_vertex)));
    if (vertices) {
        std::memcpy(vertices,geometry_.data(),bytes);
        vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLES,vertices,geometry_.size());
    }
    geometry_.clear();
}
void VitaPainter::circle(float x,float y,float radius,Color c) {
    if (radius <= 0) return;
    const int segments = std::max(8,arcSegments(radius,2*Pi));
    const float cosine = std::cos(2*Pi/segments), sine = std::sin(2*Pi/segments);
    float dx = radius, dy = 0;
    for (int i = 0; i < segments; ++i) {
        const float nx = i+1 == segments ? radius : dx*cosine-dy*sine;
        const float ny = i+1 == segments ? 0 : dx*sine+dy*cosine;
        triangle({x,y,.5f,c},{x+dx,y+dy,.5f,c},{x+nx,y+ny,.5f,c});
        dx = nx; dy = ny;
    }
}
void VitaPainter::round(Rect r,float radius,Color c) {
    if (r.w <= 0 || r.h <= 0) return;
    radius = std::clamp(radius,0.f,std::min(r.w,r.h)/2);
    if (radius == 0) { rect(r,c); return; }
    const int segments = arcSegments(radius,Pi/2);
    const vita2d_color_vertex center{r.x+r.w/2,r.y+r.h/2,.5f,c};
    const vita2d_color_vertex first{r.x+r.w-radius,r.y,.5f,c};
    auto previous = first;
    for (int corner = 0; corner < 4; ++corner) {
        const float cx = corner < 2 ? r.x+r.w-radius : r.x+radius;
        const float cy = corner == 0 || corner == 3 ? r.y+radius : r.y+r.h-radius;
        for (int i = 0; i <= segments; ++i) {
            const float angle = (corner-1+i/float(segments))*Pi/2;
            const vita2d_color_vertex next{cx+radius*std::cos(angle),cy+radius*std::sin(angle),.5f,c};
            if (corner || i) triangle(center,previous,next);
            previous = next;
        }
    }
    triangle(center,previous,first);
}
void VitaPainter::ring(float x,float y,float radius,Color c) {
    if (radius <= .5f) { circle(x,y,std::max(radius,0.f),c); return; }
    const int segments = arcSegments(radius+.5f,2*Pi);
    vita2d_color_vertex outer{x+radius+.5f,y,.5f,c}, inner{x+radius-.5f,y,.5f,c};
    for (int i = 1; i <= segments; ++i) {
        const float angle = (i%segments)*2*Pi/segments;
        const float dx = std::cos(angle), dy = std::sin(angle);
        const vita2d_color_vertex nextOuter{x+dx*(radius+.5f),y+dy*(radius+.5f),.5f,c};
        const vita2d_color_vertex nextInner{x+dx*(radius-.5f),y+dy*(radius-.5f),.5f,c};
        triangle(outer,inner,nextOuter); triangle(nextOuter,inner,nextInner);
        outer = nextOuter; inner = nextInner;
    }
}
void VitaPainter::line(float x,float y,float x2,float y2,Color c) {
    const float length = std::hypot(x2-x,y2-y);
    if (length == 0) return;
    const float dx = -(y2-y)*.5f/length, dy = (x2-x)*.5f/length;
    x += .5f; y += .5f; x2 += .5f; y2 += .5f;
    triangle({x+dx,y+dy,.5f,c},{x-dx,y-dy,.5f,c},{x2+dx,y2+dy,.5f,c});
    triangle({x2+dx,y2+dy,.5f,c},{x-dx,y-dy,.5f,c},{x2-dx,y2-dy,.5f,c});
}
void VitaPainter::text(float x,float y,float size,Color c,const std::string& t) {
    flush();
    const unsigned pixels = unsigned(std::max(1.f,std::round(size)));
    if (auto* sizedFont = font(pixels)) vita2d_font_draw_text(sizedFont,int(std::round(x)),int(std::round(y)),c,pixels,t.c_str());
    else if (fallbackFont_) vita2d_pgf_draw_text(fallbackFont_,int(x),int(y),c,size/20.f,t.c_str());
}
TextBounds VitaPainter::textBounds(float size,const std::string& text) {
    if (fallbackFont_) return {0,-size*.75f,float(vita2d_pgf_text_width(fallbackFont_,size/20.f,text.c_str())),size};
    return textLayout_.bounds(unsigned(std::max(1.f,std::round(size))),text);
}
void VitaPainter::image(int id,Rect dest,Rect src) {
    flush();
    vita2d_draw_texture_part_scale(id == 0 ? live_[liveIndex_] : review_,dest.x,dest.y,src.x,src.y,src.w,src.h,dest.w/src.w,dest.h/src.h);
}
}
