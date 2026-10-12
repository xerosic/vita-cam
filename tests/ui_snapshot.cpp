#include "ui.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>

using namespace cam;
static std::string escape(std::string s) {
    for (const auto& pair : {std::pair<std::string,std::string>{"&","&amp;"},{"<","&lt;"},{">","&gt;"},{"\"","&quot;"}}) {
        size_t at = 0;
        while ((at = s.find(pair.first,at)) != std::string::npos) { s.replace(at,pair.first.size(),pair.second); at += pair.second.size(); }
    }
    return s;
}
class SvgPainter : public Painter {
    std::ofstream out;
    void color(Color c, const char* kind = "fill") {
        out << ' ' << kind << "=\"rgb(" << (c&255) << ',' << ((c>>8)&255) << ',' << ((c>>16)&255)
            << ")\" " << kind << "-opacity=\"" << ((c>>24)&255)/255.f << "\"";
    }
public:
    explicit SvgPainter(const char* path) : out(path) {
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"960\" height=\"544\" viewBox=\"0 0 960 544\">";
        out << "<defs><linearGradient id=\"scene\" x2=\"0\" y2=\"1\"><stop stop-color=\"#9caeaf\"/><stop offset=\".55\" stop-color=\"#719396\"/><stop offset=\"1\" stop-color=\"#455f55\"/></linearGradient></defs>";
    }
    ~SvgPainter() override { out << "</svg>"; }
    void rect(Rect r, Color c) override {
        out << "<rect x=\"" << r.x << "\" y=\"" << r.y << "\" width=\"" << r.w << "\" height=\"" << r.h << '"'; color(c); out << "/>";
    }
    void circle(float x, float y, float r, Color c) override {
        out << "<circle cx=\"" << x << "\" cy=\"" << y << "\" r=\"" << r << '"'; color(c); out << "/>";
    }
    void line(float x, float y, float x2, float y2, Color c) override {
        out << "<line x1=\"" << x << "\" y1=\"" << y << "\" x2=\"" << x2 << "\" y2=\"" << y2 << '"'; color(c,"stroke"); out << "/>";
    }
    void text(float x, float y, float size, Color c, const std::string& s) override {
        out << "<text x=\"" << x << "\" y=\"" << y << "\" font-size=\"" << size << "\" font-family=\"Arial,sans-serif\""; color(c); out << '>' << escape(s) << "</text>";
    }
    void image(int, Rect r, Rect) override {
        out << "<svg x=\"" << r.x << "\" y=\"" << r.y << "\" width=\"" << r.w << "\" height=\"" << r.h << "\" viewBox=\"0 0 640 480\">";
        out << "<rect width=\"640\" height=\"480\" fill=\"url(#scene)\"/>"
            "<path d=\"M0 212L80 170L146 205L290 151L399 207L512 166L640 194V480H0Z\" fill=\"#5b756d\"/>"
            "<path d=\"M0 288L92 263L194 306L320 239L489 282L640 244V480H0Z\" fill=\"#3f5c4e\"/>"
            "<path d=\"M30 480L246 289H320L411 480Z\" fill=\"#b1a183\"/>"
            "<path d=\"M0 413L185 366L132 480H0Z\" fill=\"#7c8c67\"/>"
            "<text x=\"320\" y=\"28\" text-anchor=\"middle\" fill=\"#ecf3ed\" font-family=\"Arial\" font-size=\"12\">UI PREVIEW / SIMULATED SCENE</text></svg>";
    }
};
int main(int argc, char** argv) {
    UiState u;
    u.ready = true; u.hasReview = true; u.battery = 82; u.clock = "16:24";
    for (int i = 0; i < 64; ++i) u.analysis.bins[i] = unsigned(30+(i*37)%150);
    u.analysis.peak = 200;
    if (argc > 2) {
        const std::string mode = argv[2];
        if (mode == "settings") u.screen = Screen::Settings;
        if (mode == "view") { u.screen = Screen::Settings; u.tab = 1; }
        if (mode == "review") u.screen = Screen::Review;
        if (mode == "help") u.screen = Screen::Help;
        if (mode == "clean") u.clean = true;
        if (mode == "error") { u.ready = false; u.cameraError = "Camera read: 0x802e000a"; }
    }
    SvgPainter painter(argc > 1 ? argv[1] : "ui.svg");
    drawUi(painter,u);
    std::cout << "Rendered shared C++ UI\n";
}
