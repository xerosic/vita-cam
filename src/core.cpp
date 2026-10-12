#include "core.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace cam {
int stepEv(int value, int direction) {
    auto it = std::min_element(EvSteps.begin(), EvSteps.end(), [value](int a, int b) { return std::abs(a-value) < std::abs(b-value); });
    return EvSteps[std::clamp(int(it-EvSteps.begin()) + direction, 0, int(EvSteps.size())-1)];
}
int cycleValue(int value, const int* values, size_t count, int direction) {
    if (!count) return value;
    const auto it = std::find(values, values+count, value);
    const int index = it == values+count ? 0 : int(it-values);
    return values[(index + direction % int(count) + int(count)) % int(count)];
}
std::string evLabel(int value) {
    char label[16];
    std::snprintf(label, sizeof(label), "%+.1f", value / 10.0);
    return label;
}
std::string wbLabel(int value) {
    for (size_t i = 0; i < WhiteBalances.size(); ++i) if (WhiteBalances[i] == value) return WbNames[i];
    return "Unknown";
}
std::string isoLabel(int value) { return value == 1 ? "Auto" : std::to_string(value); }
uint8_t luminance(Pixel p) { return uint8_t((77u*p.r + 150u*p.g + 29u*p.b) >> 8); }
Pixel processPixel(Pixel p, Look look) {
    const auto clip = [](int v) { return uint8_t(std::clamp(v, 0, 255)); };
    if (look == Look::Mono) p.r = p.g = p.b = luminance(p);
    if (look == Look::Warm) { p.r = clip(p.r * 106 / 100 + 3); p.b = clip(p.b * 92 / 100); }
    if (look == Look::Cool) { p.r = clip(p.r * 94 / 100); p.b = clip(p.b * 105 / 100 + 3); }
    p.a = 255;
    return p;
}
void processFrame(const std::vector<Pixel>& source, std::vector<Pixel>& target, Look look, bool mirror) {
    target.resize(source.size());
    for (size_t i = 0; i < source.size(); ++i) {
        const size_t row = i / Width * Width;
        const size_t src = mirror && row + Width <= source.size() ? row + Width - 1 - i % Width : i;
        target[i] = processPixel(source[src], look);
    }
}
Analysis analyze(const std::vector<Pixel>& pixels) {
    Analysis a;
    unsigned samples = 0, high = 0, low = 0;
    for (size_t i = 0; i < pixels.size(); i += 4) {
        const auto p = pixels[i];
        const auto y = luminance(p);
        ++a.bins[y/4]; ++samples;
        if (std::max({p.r, p.g, p.b}) >= 250) ++high;
        if (y <= 5) ++low;
    }
    a.peak = *std::max_element(a.bins.begin(), a.bins.end());
    if (samples) { a.highlights = 100.f*high/samples; a.shadows = 100.f*low/samples; }
    return a;
}
std::string settingsText(const Settings& s) {
    std::ostringstream out;
    out << "version=1\ndevice=" << s.device << "\nev=" << s.ev << "\nwb=" << s.wb << "\niso=" << s.iso
        << "\nflicker=" << s.flicker << "\ntimer=" << s.timer << "\nlook=" << int(s.look) << "\ngrid=" << int(s.grid)
        << "\nhistogram=" << s.histogram << "\nzebras=" << s.zebras << "\nshadows=" << s.shadows << "\nmirror=" << s.mirror << '\n';
    return out.str();
}
Settings parseSettings(const std::string& text) {
    Settings s;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        const auto equal = line.find('=');
        if (equal == std::string::npos) continue;
        const auto key = line.substr(0, equal);
        std::istringstream number(line.substr(equal+1));
        int value; char extra;
        if (!(number >> value) || number >> extra) continue;
        const auto oneOf = [value](const auto& values) { return std::find(values.begin(), values.end(), value) != values.end(); };
        if (key == "device" && value >= 0 && value <= 1) s.device = value;
        if (key == "ev" && oneOf(EvSteps)) s.ev = value;
        if (key == "wb" && oneOf(WhiteBalances)) s.wb = value;
        if (key == "iso" && oneOf(IsoModes)) s.iso = value;
        if (key == "flicker" && value >= 1 && value <= 3) s.flicker = value;
        if (key == "timer" && oneOf(Timers)) s.timer = value;
        if (key == "look" && value >= 0 && value < 4) s.look = Look(value);
        if (key == "grid" && value >= 0 && value < 3) s.grid = Grid(value);
        if (value == 0 || value == 1) {
            if (key == "histogram") s.histogram = value;
            if (key == "zebras") s.zebras = value;
            if (key == "shadows") s.shadows = value;
            if (key == "mirror") s.mirror = value;
        }
    }
    return s;
}
std::string metadata(const Capture& c) {
    const auto& s = c.settings;
    std::ostringstream out;
    out << "{\n  \"app\": \"Vita Camera\",\n  \"version\": \"" << Version << "\",\n"
        << "  \"width\": " << Width << ", \"height\": " << Height << ",\n"
        << "  \"camera\": \"" << (s.device ? "rear" : "front") << "\",\n"
        << "  \"frame_id\": " << c.frame.id << ",\n  \"camera_timestamp\": " << c.frame.timestamp << ",\n"
        << "  \"requested\": {\"ev_tenths\": " << s.ev << ", \"iso_mode\": " << s.iso
        << ", \"white_balance_mode\": " << s.wb << ", \"anti_flicker_mode\": " << s.flicker
        << ", \"fps\": " << c.fps << "},\n"
        << "  \"processing\": \"" << LookNames[int(s.look)] << "\",\n"
        << "  \"original_processing\": \"none\",\n  \"export_mirrored\": false,\n"
        << "  \"preview_mirrored\": " << ((!s.device && s.mirror) ? "true" : "false") << ",\n"
        << "  \"encoding\": \"PNG RGB8\"\n}\n";
    return out.str();
}
}
