#include "ui.hpp"
#include <algorithm>
#include <cmath>

namespace cam {
static constexpr Color White = rgba(241, 250, 255);
static constexpr Color Dim = rgba(145, 184, 218);
static constexpr Color Cyan = rgba(101, 220, 255);
static constexpr Color Glass = rgba(8, 30, 62, 225);
void Painter::centeredText(Rect r,float size,Color color,const std::string& value) {
    const auto bounds = textBounds(size,value);
    text(r.x+(r.w-bounds.width)/2-bounds.x,r.y+(r.h-bounds.height)/2-bounds.y,size,color,value);
}
void Painter::gradient(Rect r, Color top, Color bottom) {
    for (int y = 0; y < int(r.h); y += 2) {
        const float t = float(y) / std::max(1.f, r.h-1);
        Color c = 0;
        for (int s = 0; s < 32; s += 8) c |= unsigned(((top>>s)&255)*(1-t) + ((bottom>>s)&255)*t)<<s;
        rect({r.x, r.y+y, r.w, std::min(2.f, r.h-y)}, c);
    }
}
static void cameraIcon(Painter& p, float x, float y, float scale, Color color) {
    p.round({x-18*scale, y-12*scale, 36*scale, 25*scale}, 4*scale, color);
    p.round({x-9*scale, y-17*scale, 16*scale, 9*scale}, 2*scale, color);
    p.circle(x, y, 9*scale, rgba(9,42,77));
    p.ring(x, y, 6*scale, Cyan);
    p.circle(x-1.5f*scale, y-2*scale, 2*scale, White);
}
static void buttonSurface(Painter& p, Rect r, bool active = false) {
    const float radius = std::min(r.w,r.h)/2;
    p.round({r.x,r.y+2,r.w,r.h},radius,rgba(0,7,24,70));
    p.round(r,radius,active ? rgba(87,174,209) : rgba(52,90,123));
    p.round({r.x+1,r.y+1,r.w-2,r.h-2},radius-1,active ? rgba(35,124,177) : rgba(29,65,100));
}
static void pill(Painter& p, Rect r, const std::string& label, bool active = false) {
    buttonSurface(p,r,active);
    p.centeredText(r,18,White,label);
}
static void stepButton(Painter& p, Rect r, bool increase) {
    buttonSurface(p,r);
    const float x = r.x+r.w/2, y = r.y+r.h/2;
    p.round({x-6,y-1,12,2},1,White);
    if (increase) p.round({x-1,y-6,2,12},1,White);
}
static void chevron(Painter& p, float x, float y, Color color) {
    p.line(x-2,y-3,x+2,y,color);
    p.line(x+2,y,x-2,y+3,color);
}
int settingsRows(int tab) { return tab == 0 ? 7 : 6; }
std::string settingName(int tab, int row) {
    static const char* shooting[]{"White balance", "ISO request", "Anti-flicker", "Color look", "Self-timer", "Reset camera", "Controls"};
    static const char* view[]{"Composition guide", "Graph", "Highlight zebras", "Shadow warning", "Selfie preview", "Reset view"};
    return row < 0 || row >= settingsRows(tab) ? "" : tab == 0 ? shooting[row] : view[row];
}
std::string settingValue(const UiState& u, int row) {
    const auto& s = u.settings;
    if (!u.tab) {
        switch (row) {
        case 0: return wbLabel(s.wb);
        case 1: return isoLabel(s.iso);
        case 2: return s.flicker == 1 ? "Auto" : s.flicker == 2 ? "50 Hz" : "60 Hz";
        case 3: return LookNames[int(s.look)];
        case 4: return s.timer ? std::to_string(s.timer)+" sec" : "Off";
        case 5: return "Restore defaults";
        default: return "Open";
        }
    }
    switch (row) {
    case 0: return s.grid == Grid::Off ? "Off" : s.grid == Grid::Thirds ? "Thirds" : "Square";
    case 1: return GraphNames[int(s.graph)];
    case 2: return s.zebras ? "On" : "Off";
    case 3: return s.shadows ? "On" : "Off";
    case 4: return s.mirror ? "Mirrored" : "Natural";
    default: return "Restore defaults";
    }
}
Hit hitTest(const std::vector<Hit>& hits, float x, float y) {
    for (auto i = hits.rbegin(); i != hits.rend(); ++i) if (i->rect.contains(x, y)) return *i;
    return {{0,0,0,0}, Action::None};
}
std::vector<Hit> drawUi(Painter& p, const UiState& u) {
    std::vector<Hit> hits;
    const auto hit = [&](Rect r, Action a, int value = 0) { hits.push_back({r, a, value}); };
    p.gradient({0,0,960,544}, rgba(24,74,125), rgba(4,17,42));
    static const auto waves = [] {
        std::array<std::array<float,121>,3> points{};
        for (int i = 0; i < 3; ++i) for (int x = 0; x <= 120; ++x)
            points[i][x] = 340.f+i*36+std::sin(x*8/195.f+i*.3f)*75;
        return points;
    }();
    for (int i = 0; i < 3; ++i) {
        for (int x = 0; x < 960; x += 8) {
            p.line(float(x), waves[i][x/8], float(x+8), waves[i][x/8+1], rgba(67,152,210,40-i*9));
        }
    }
    if (u.screen == Screen::Share) {
        p.text(54,70,32,White,"Share photos");
        pill(p,{797,36,110,44},"Back"); hit({785,27,140,65},Action::Back);
        p.round({112,111,736,369},20,Glass);
        if (u.sharing) {
            p.rect({145,145,248,248},rgba(255,255,255));
            if (u.qr.size) {
                const int scale = 248/(u.qr.size+8), extent = scale*u.qr.size;
                const int left = 145+(248-extent)/2, top = 145+(248-extent)/2;
                for (int y = 0; y < u.qr.size; ++y) for (int x = 0; x < u.qr.size; ++x)
                    if (u.qr.modules[y*u.qr.size+x]) p.rect({float(left+x*scale),float(top+y*scale),float(scale),float(scale)},rgba(0,0,0));
            }
            p.text(429,172,25,White,"Scan with iPhone Camera");
            p.text(429,210,18,Cyan,std::to_string(u.photoCount)+" photos");
            p.text(429,251,18,White,"Same Wi-Fi or iPhone hotspot.");
            p.text(429,289,18,Dim,"Open the link, then hold a photo");
            p.text(429,316,18,Dim,"to save it to Photos.");
            p.text(429,369,16,Dim,"Keep this screen open while saving.");
            p.text(145,440,13,Cyan,u.shareUrl);
        } else {
            p.text(150,188,25,White,"Connect to Wi-Fi");
            p.text(150,231,18,Dim,u.shareError);
            p.text(150,273,18,White,"At home: connect both devices to the same Wi-Fi.");
            p.text(150,307,18,White,"On the go: enable Personal Hotspot on your iPhone.");
            p.text(150,335,16,Dim,"Join it in Vita Settings, then return here and retry.");
            pill(p,{150,358,132,44},"Retry"); hit({138,346,160,68},Action::Share);
        }
        p.text(54,524,14,Dim,"CIRCLE  Back");
        return hits;
    }
    const bool review = u.screen == Screen::Review;
    const Rect view = u.clean && u.screen == Screen::Camera ? FullViewport : Viewport;
    p.rect(view, rgba(3,8,14));
    if ((u.ready && !review) || (review && u.reviewReady)) {
        const auto crop = review ? zoomCrop(u.reviewZoom,u.panX,u.panY) : zoomCrop(u.zoom);
        p.image(review ? 1 : 0, view, {crop.x,crop.y,crop.w,crop.h});
    } else {
        cameraIcon(p,view.x+view.w/2,223,1.7f,rgba(66,103,134));
        p.centeredText({view.x,264,view.w,32},23,White,review ? (u.photoCount ? "Photo unavailable" : "No photographs yet") : "Waiting for camera");
        p.centeredText({view.x,301,view.w,28},17,Dim,review ? "Press Circle to return to the camera" : "Press L to switch or retry the camera");
        if (!u.cameraError.empty() && !review) p.centeredText({view.x,341,view.w,28},17,rgba(255,189,143),u.cameraError);
    }
    if (!review && u.ready && u.screen == Screen::Camera) {
        if (u.settings.grid == Grid::Thirds && !u.clean) {
            for (int i = 1; i < 3; ++i) {
                p.line(view.x+view.w*i/3, view.y, view.x+view.w*i/3, view.y+view.h, rgba(255,255,255,90));
                p.line(view.x, view.y+view.h*i/3, view.x+view.w, view.y+view.h*i/3, rgba(255,255,255,90));
            }
        }
        if (u.settings.grid == Grid::Square && !u.clean) {
            const float margin = (view.w-view.h)/2;
            p.rect({view.x,view.y,margin,view.h}, rgba(0,0,0,95));
            p.rect({view.x+view.w-margin,view.y,margin,view.h}, rgba(0,0,0,95));
            p.line(view.x+margin,view.y,view.x+margin,view.y+view.h,rgba(255,255,255,120));
            p.line(view.x+view.w-margin,view.y,view.x+view.w-margin,view.y+view.h,rgba(255,255,255,120));
        }
        if (!u.clean && u.settings.graph != Graph::Off) {
            p.round({160,424,166,80}, 7, rgba(4,15,31,195));
            p.text(171,442,12,Dim,GraphNames[int(u.settings.graph)]);
            if (u.settings.graph == Graph::Waveform) {
                for (int y = 0; y < 32; ++y) for (int x = 0; x < 64; ++x) {
                    const unsigned count = u.analysis.waveform[y*64+x];
                    if (count) p.rect({172+x*2.2f,448+y*1.45f,2.2f,1.45f},rgba(101,220,255,60+195*std::sqrt(float(count)/std::max(1u,u.analysis.waveformPeak))));
                }
            } else for (int i = 0; i < 64; ++i) {
                if (u.settings.graph == Graph::Luminance) {
                    const float height = u.analysis.peak ? 46.f*u.analysis.bins[i]/u.analysis.peak : 0;
                    p.rect({172+i*2.2f, 495-height, 1.8f, height}, Cyan);
                } else {
                    const Color colors[]{rgba(255,115,115,190),rgba(110,240,161,190),rgba(104,163,255,190)};
                    for (int channel = 0; channel < 3; ++channel) {
                        const float height = u.analysis.rgbPeak ? 46.f*u.analysis.rgb[channel][i]/u.analysis.rgbPeak : 0;
                        p.rect({172+i*2.2f,495-height,1.8f,height},colors[channel]);
                    }
                }
            }
            p.line(171,496,314,496,Dim);
        }
        hit(view, Action::Clean);
    }
    if (!u.clean || u.screen != Screen::Camera) {
        p.gradient({0,0,960,40},rgba(30,83,132),rgba(14,44,81));
        p.line(0,39,960,39,rgba(116,190,233,90));
        cameraIcon(p, 26, 20, .55f, White);
        p.text(47,27,22,White,"Vita Camera");
        p.text(258,26,15,Dim,review ? "PHOTOS" : "PHOTO");
        p.text(624,26,15,Dim,"640 x 480");
        p.text(798,27,18,White,u.clock);
        if (u.battery >= 0) {
            p.round({867,13,31,15},2,rgba(128,171,201));
            p.rect({869,15,27,11},rgba(12,40,70));
            p.rect({870,16,25.f*std::clamp(u.battery,0,100)/100,9},u.battery < 15 ? rgba(255,174,107) : Cyan);
            p.rect({898,17,3,7},Dim);
            p.text(908,26,14,Dim,std::to_string(u.battery)+"%");
        }
        p.rect({0,520,960,24},rgba(1,10,29,180));
        p.text(17,537,13,Dim,review ? "LEFT/RIGHT  Browse     RIGHT STICK  Zoom     LEFT STICK  Pan" : "R  Shutter     UP/DOWN  Graph     RIGHT STICK  Zoom     TRIANGLE  Settings");
        p.text(692,537,13,Dim,review ? "SELECT  Original" : "SQUARE  Review    CIRCLE  Clean");
        if (review) {
            p.text(19,78,13,Dim,std::to_string(u.photoIndex)+" / "+std::to_string(u.photoCount));
            p.text(19,108,22,White,zoomLabel(u.reviewZoom));
            pill(p,{16,136,110,42},u.reviewZoom > 1 ? "Fit" : "Zoom",u.reviewZoom > 1); hit({10,129,124,56},Action::Zoom);
            p.text(17,217,14,Dim,"PNG / RGB");
            p.text(17,241,14,Dim,"640 x 480");
            p.text(17,297,14,Cyan,"Original kept");
            pill(p,{16,335,110,42},"Previous"); hit({8,328,126,56},Action::PreviousPhoto);
            pill(p,{16,395,110,42},"Next"); hit({8,388,126,56},Action::NextPhoto);
            pill(p,{801,59,140,44},"Back"); hit({793,48,158,65},Action::Back);
            pill(p,{801,121,140,44},"Share"); hit({793,110,158,65},Action::Share);
            p.text(805,421,14,Dim,"CAPTURED");
            p.text(805,447,14,White,u.lastName.substr(0,10));
            if (u.lastName.size() > 12) p.text(805,469,14,Dim,u.lastName.substr(12,8));
        } else {
            p.text(19,74,13,Dim,"EXPOSURE");
            p.text(22,117,36,White,evLabel(u.settings.ev));
            p.text(102,112,13,Dim,"EV");
            stepButton(p,{16,134,48,42},false); stepButton(p,{78,134,48,42},true);
            hit({8,124,64,62},Action::EvDown); hit({72,124,64,62},Action::EvUp);
            const auto control = [&](float y, const char* label, const std::string& value, Action a) {
                p.line(18,y-17,126,y-17,rgba(78,122,166,80));
                p.text(19,y+1,12,Dim,label);
                p.text(19,y+30, value.size() > 10 ? 16 : 20,White,value);
                chevron(p,125,y+23,Cyan);
                hit({7,y-9,131,65},a);
            };
            control(205,"GRAPH",GraphNames[int(u.settings.graph)],Action::GraphNext);
            control(285,"DIGITAL ZOOM",zoomLabel(u.zoom),Action::Zoom);
            control(365,"SELF-TIMER",u.settings.timer ? std::to_string(u.settings.timer)+" sec" : "Off",Action::Timer);
            pill(p,{16,452,110,42},"Share"); hit({8,442,128,62},Action::Share);
            pill(p,{802,57,141,40},"Settings"); hit({793,46,162,61},Action::Settings);
            pill(p,{802,121,141,40},u.settings.device ? "Rear camera" : "Front camera"); hit({793,109,162,67},Action::SwitchCamera);
            p.centeredText({802,177,141,22},13,Dim,u.compare ? "ORIGINAL" : "STILL PHOTO");
            p.circle(872,272,60,rgba(3,16,39,150));
            p.circle(872,269,59,rgba(47,113,162));
            p.ring(872,269,57,rgba(179,231,255));
            p.circle(872,269,49,rgba(7,30,64));
            p.circle(872,268,44,u.countdown ? rgba(235,169,74) : rgba(226,243,250));
            p.circle(872,259,33,u.countdown ? rgba(253,190,94) : rgba(244,253,255));
            cameraIcon(p,872,268,1.f,rgba(42,94,132));
            hit({799,198,146,147},Action::Shutter);
            p.centeredText({799,335,146,32},16,White,u.countdown ? "Cancel" : u.failedSave ? "Retry save" : "Capture");
            p.round({814,387,116,89},7,rgba(65,103,137));
            p.round({815,388,114,87},6,rgba(5,18,38));
            if (u.hasReview) p.image(2,{818,391,108,81},{0,0,640,480});
            else cameraIcon(p,872,428,.7f,rgba(56,93,127));
            p.centeredText({814,479,116,26},15,Dim,"Photos"); hit({797,378,154,133},Action::Review);
        }
    } else hit({0,0,960,544},Action::Clean);
    if (u.countdown && u.screen == Screen::Camera) {
        p.circle(464,256,51,rgba(4,26,48,210));
        p.centeredText({413,205,102,102},58,White,std::to_string(u.countdown));
    }
    if (u.compare) {
        p.round({405,51,125,29},14,Glass);
        p.centeredText({405,51,125,29},14,White,"ORIGINAL");
    }
    if (u.screen == Screen::Settings) {
        hits.clear();
        p.rect({0,40,960,480},rgba(0,9,25,160));
        p.round({177,52,606,455},17,rgba(2,11,30,160));
        p.round({177,48,606,455},17,rgba(16,47,83,252));
        p.text(203,85,27,White,"Camera settings");
        pill(p,{667,60,91,36},"Done"); hit({655,51,115,53},Action::Back);
        pill(p,{200,106,171,38},"Shooting",u.tab == 0); hit({198,101,174,46},Action::Tab,0);
        pill(p,{385,106,171,38},"Viewfinder",u.tab == 1); hit({383,101,175,46},Action::Tab,1);
        for (int row = 0; row < settingsRows(u.tab); ++row) {
            const float y = 153+row*42;
            if (u.row == row) p.round({195,y,570,40},8,rgba(34,111,163));
            p.text(211,y+26,19,White,settingName(u.tab,row));
            p.text(527,y+26,18,u.row == row ? White : Cyan,settingValue(u,row));
            chevron(p,749,y+20,Dim);
            hit({195,y,570,42},Action::Row,row);
        }
        p.text(204,483,14,Dim,u.tab ? "Saved selfies are unmirrored. Guides are preview only." : "Color looks save a copy. The original is always kept.");
    }
    if (u.screen == Screen::Help) {
        hits.clear();
        p.rect({0,40,960,480},rgba(0,9,25,200));
        p.round({176,58,608,431},17,rgba(15,47,82));
        p.text(204,100,28,White,"Controls");
        const char* controls[][2] = {{"R / Cross", "Take a photograph"},{"Left / Right", "Exposure compensation"},{"Up / Down", "Previous / next graph"},{"Right stick", "Digital zoom"},{"Square / Triangle", "Photos / settings"},{"Circle", "Clean view / back / cancel timer"},{"Hold Select", "Compare with original"},{"L", "Switch camera"}};
        for (int i = 0; i < 8; ++i) {
            p.text(208,147+i*33,17,Cyan,controls[i][0]);
            p.text(414,147+i*33,17,White,controls[i][1]);
        }
        pill(p,{634,430,126,39},"Back"); hit({620,417,154,65},Action::Back);
        p.text(207,461,14,Dim,"Photographs: ux0:data/VitaCamera/photos");
    }
    if (!u.toast.empty()) {
        const float width = std::min(608.f,36.f+p.textBounds(18,u.toast).width);
        p.round({480-width/2,365,width,45},20,rgba(5,27,50,240));
        p.centeredText({480-width/2,365,width,45},18,White,u.toast);
    } else if (u.saving) {
        p.round({399,363,161,40},20,Glass);
        p.centeredText({399,363,161,40},17,White,"Saving photo...");
    }
    if (u.flash > 0) p.rect(view,rgba(255,255,255,unsigned(std::clamp(u.flash,0.f,1.f)*160)));
    return hits;
}
}
