#include "ui.hpp"
#include <algorithm>
#include <cmath>

namespace cam {
static constexpr Color White = rgba(241, 250, 255);
static constexpr Color Dim = rgba(145, 184, 218);
static constexpr Color Cyan = rgba(101, 220, 255);
static constexpr Color Glass = rgba(8, 30, 62, 225);
void Painter::round(Rect r, float radius, Color color) {
    radius = std::min({radius, r.w/2, r.h/2});
    rect({r.x+radius, r.y, r.w-2*radius, r.h}, color);
    rect({r.x, r.y+radius, radius, r.h-2*radius}, color);
    rect({r.x+r.w-radius, r.y+radius, radius, r.h-2*radius}, color);
    circle(r.x+radius, r.y+radius, radius, color);
    circle(r.x+r.w-radius, r.y+radius, radius, color);
    circle(r.x+radius, r.y+r.h-radius, radius, color);
    circle(r.x+r.w-radius, r.y+r.h-radius, radius, color);
}
void Painter::gradient(Rect r, Color top, Color bottom) {
    for (int y = 0; y < int(r.h); y += 2) {
        const float t = float(y) / std::max(1.f, r.h-1);
        Color c = 0;
        for (int s = 0; s < 32; s += 8) c |= unsigned(((top>>s)&255)*(1-t) + ((bottom>>s)&255)*t)<<s;
        rect({r.x, r.y+y, r.w, std::min(2.f, r.h-y)}, c);
    }
}
static void ring(Painter& p, float x, float y, float r, Color color) {
    for (int i = 0; i < 64; ++i) {
        const float a = i*6.2831853f/64, b = (i+1)*6.2831853f/64;
        p.line(x+std::cos(a)*r, y+std::sin(a)*r, x+std::cos(b)*r, y+std::sin(b)*r, color);
    }
}
static void cameraIcon(Painter& p, float x, float y, float scale, Color color) {
    p.round({x-18*scale, y-12*scale, 36*scale, 25*scale}, 4*scale, color);
    p.round({x-9*scale, y-17*scale, 16*scale, 9*scale}, 2*scale, color);
    p.circle(x, y, 9*scale, rgba(9,42,77));
    ring(p, x, y, 6*scale, Cyan);
    p.circle(x-1.5f*scale, y-2*scale, 2*scale, White);
}
static void pill(Painter& p, Rect r, const std::string& label, bool active = false) {
    p.round({r.x, r.y+2, r.w, r.h}, r.h/2, rgba(0, 7, 24, 100));
    p.round(r, r.h/2, active ? rgba(35, 131, 191) : rgba(31, 69, 107));
    p.line(r.x+r.h/2, r.y+1, r.x+r.w-r.h/2, r.y+1, active ? Cyan : rgba(83, 126, 162));
    p.text(r.x+16, r.y+r.h/2+6, 18, White, label);
}
int settingsRows(int tab) { return tab == 0 ? 7 : 6; }
std::string settingName(int tab, int row) {
    static const char* shooting[]{"White balance", "ISO request", "Anti-flicker", "Color look", "Self-timer", "Reset camera", "Controls"};
    static const char* view[]{"Composition guide", "Histogram", "Highlight zebras", "Shadow warning", "Selfie preview", "Reset view"};
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
    case 1: return s.histogram ? "On" : "Off";
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
    for (int i = 0; i < 3; ++i) {
        for (int x = 0; x < 960; x += 8) {
            const auto wave = [i](int n) { return 340.f + i*36 + std::sin(n/195.f + i*.3f)*75; };
            p.line(float(x), wave(x), float(x+8), wave(x+8), rgba(67,152,210,40-i*9));
        }
    }
    const bool review = u.screen == Screen::Review;
    const Rect view = u.clean && u.screen == Screen::Camera ? FullViewport : Viewport;
    p.rect(view, rgba(3,8,14));
    if ((u.ready && !review) || (review && u.hasReview)) {
        Rect crop{0,0,640,480};
        if (review && u.zoom) crop = {u.panX, u.panY, 320, 240};
        p.image(review ? 1 : 0, view, crop);
    } else {
        cameraIcon(p, 464, 223, 1.7f, rgba(66,103,134));
        p.text(325, 283, 23, White, review ? "No photographs yet" : "Waiting for camera");
        p.text(294, 316, 17, Dim, review ? "Press Circle to return to the camera" : "Press L to switch or retry the camera");
        if (!u.cameraError.empty() && !review) p.text(292, 356, 17, rgba(255,189,143), u.cameraError);
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
        if (!u.clean && u.settings.histogram) {
            p.round({160,424,166,80}, 7, rgba(4,15,31,195));
            p.text(171,442,12,Dim,"LUMINANCE");
            for (int i = 0; i < 64; ++i) {
                const float height = u.analysis.peak ? 46.f*u.analysis.bins[i]/u.analysis.peak : 0;
                p.rect({172+i*2.2f, 495-height, 1.8f, height}, Cyan);
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
        p.text(258,26,15,Dim,review ? "REVIEW" : "PHOTO");
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
        p.text(17,537,13,Dim,review ? "CIRCLE  Back     CROSS  Zoom     D-PAD  Pan" : "R  Shutter     L  Camera     D-PAD  Adjust     TRIANGLE  Settings");
        p.text(692,537,13,Dim,review ? "SELECT  Original" : "SQUARE  Review    CIRCLE  Clean");
        if (review) {
            p.text(19,78,13,Dim,"LAST SHOT");
            p.text(19,108,22,White,u.zoom ? "2x detail" : "1:1 pixels");
            pill(p,{16,136,110,42},u.zoom ? "Fit" : "Zoom",u.zoom); hit({10,129,124,56},Action::Zoom);
            p.text(17,217,14,Dim,"PNG / RGB");
            p.text(17,241,14,Dim,"640 x 480");
            p.text(17,297,14,Cyan,"Original kept");
            pill(p,{801,59,140,44},"Back"); hit({793,48,158,65},Action::Back);
            p.text(805,421,14,Dim,"LAST CAPTURE");
            p.text(805,447,14,White,u.failedSave ? "Save pending" : "Lossless PNG");
        } else {
            p.text(19,74,13,Dim,"EXPOSURE");
            p.text(22,117,36,White,evLabel(u.settings.ev));
            p.text(102,112,13,Dim,"EV");
            pill(p,{16,134,48,42},"-"); pill(p,{78,134,48,42},"+");
            hit({8,124,64,62},Action::EvDown); hit({72,124,64,62},Action::EvUp);
            const auto control = [&](float y, const char* label, const std::string& value, Action a) {
                p.line(18,y-17,126,y-17,rgba(78,122,166,80));
                p.text(19,y+1,12,Dim,label);
                p.text(19,y+30, value.size() > 10 ? 16 : 20,White,value);
                p.text(122,y+28,15,Cyan,">");
                hit({7,y-9,131,65},a);
            };
            control(205,"WHITE BALANCE",wbLabel(u.settings.wb),Action::Wb);
            control(285,"COLOR LOOK",LookNames[int(u.settings.look)],Action::Look);
            control(365,"SELF-TIMER",u.settings.timer ? std::to_string(u.settings.timer)+" sec" : "Off",Action::Timer);
            p.text(19,457,12,Dim,"ISO REQUEST");
            p.text(19,484,21,White,isoLabel(u.settings.iso));
            pill(p,{802,57,141,40},"Settings"); hit({793,46,162,61},Action::Settings);
            pill(p,{802,121,141,40},u.settings.device ? "Rear camera" : "Front camera"); hit({793,109,162,67},Action::SwitchCamera);
            p.text(822,193,13,Dim,u.compare ? "ORIGINAL" : "STILL PHOTO");
            p.circle(872,272,60,rgba(3,16,39,150));
            p.circle(872,269,59,rgba(47,113,162));
            ring(p,872,269,57,rgba(179,231,255));
            p.circle(872,269,49,rgba(7,30,64));
            p.circle(872,268,44,u.countdown ? rgba(235,169,74) : rgba(226,243,250));
            p.circle(872,259,33,u.countdown ? rgba(253,190,94) : rgba(244,253,255));
            cameraIcon(p,872,268,1.f,rgba(42,94,132));
            hit({799,198,146,147},Action::Shutter);
            p.text(839,355,16,White,u.countdown ? "Cancel" : u.failedSave ? "Retry save" : "Capture");
            p.round({814,387,116,85},7,rgba(112,164,204,180));
            p.rect({818,391,108,77},rgba(5,18,38));
            if (u.hasReview) p.image(2,{820,392,104,78},{0,0,640,480});
            else cameraIcon(p,872,428,.7f,rgba(56,93,127));
            p.text(839,495,15,Dim,"Last shot"); hit({797,378,154,133},Action::Review);
        }
    } else hit({0,0,960,544},Action::Clean);
    if (u.countdown && u.screen == Screen::Camera) {
        p.circle(464,256,51,rgba(4,26,48,210));
        p.text(u.countdown > 9 ? 429 : 446,277,58,White,std::to_string(u.countdown));
    }
    if (u.compare) {
        p.round({405,51,125,29},14,Glass);
        p.text(421,71,14,White,"ORIGINAL");
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
            p.text(744,y+26,18,Dim,">");
            hit({195,y,570,42},Action::Row,row);
        }
        p.text(204,483,14,Dim,u.tab ? "Guides stay out of your photos. Saved selfies are unmirrored." : "Color looks save a separate copy. The original is always kept.");
    }
    if (u.screen == Screen::Help) {
        hits.clear();
        p.rect({0,40,960,480},rgba(0,9,25,200));
        p.round({176,58,608,431},17,rgba(15,47,82));
        p.text(204,100,28,White,"Make yourself at home");
        const char* controls[][2] = {{"R / Cross", "Take a photograph"},{"Left / Right", "Exposure compensation"},{"Up / Down", "White balance / color look"},{"L", "Switch camera"},{"Square / Triangle", "Last shot / settings"},{"Circle", "Clean view / back / cancel timer"},{"Hold Select", "Compare with original"},{"Start", "Show these controls"}};
        for (int i = 0; i < 8; ++i) {
            p.text(208,147+i*33,17,Cyan,controls[i][0]);
            p.text(414,147+i*33,17,White,controls[i][1]);
        }
        pill(p,{634,430,126,39},"Back"); hit({620,417,154,65},Action::Back);
        p.text(207,461,14,Dim,"Photographs: ux0:data/VitaCamera/photos");
    }
    if (!u.toast.empty()) {
        const float width = std::min(608.f, 36.f+u.toast.size()*9.5f);
        p.round({480-width/2,365,width,45},20,rgba(5,27,50,240));
        p.text(498-width/2,394,18,White,u.toast);
    } else if (u.saving) {
        p.round({399,363,161,40},20,Glass);
        p.text(423,389,17,White,"Saving photo...");
    }
    if (u.flash > 0) p.rect(view,rgba(255,255,255,unsigned(std::clamp(u.flash,0.f,1.f)*160)));
    return hits;
}
}
