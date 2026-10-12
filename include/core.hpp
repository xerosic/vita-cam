#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace cam {
constexpr int Width = 640;
constexpr int Height = 480;
constexpr const char* Version = "0.1.3";
constexpr std::array<int, 17> EvSteps{-20, -17, -15, -13, -10, -7, -5, -3, 0, 3, 5, 7, 10, 13, 15, 17, 20};
constexpr std::array<int, 4> WhiteBalances{0, 1, 2, 4};
constexpr std::array<int, 4> IsoModes{1, 100, 200, 400};
constexpr std::array<int, 3> Timers{0, 2, 10};
constexpr std::array<const char*, 4> WbNames{"Auto", "Daylight", "Fluorescent", "Tungsten"};
constexpr std::array<const char*, 4> LookNames{"Original", "Mono", "Warm", "Cool"};
enum class Look { Original, Mono, Warm, Cool };
enum class Grid { Off, Thirds, Square };
enum class Graph { Luminance, Rgb, Waveform, Off };
constexpr std::array<const char*, 4> GraphNames{"Luminance", "RGB", "Waveform", "Off"};
constexpr float MaxZoom = 4.f;
struct ImageCrop { float x, y, w, h; };
struct Pixel { uint8_t r, g, b, a; };
static_assert(sizeof(Pixel) == 4);
struct Settings {
    int device = 1;
    int ev = 0;
    int wb = 0;
    int iso = 1;
    int flicker = 1;
    int timer = 0;
    Look look = Look::Original;
    Grid grid = Grid::Thirds;
    Graph graph = Graph::Luminance;
    bool zebras = false;
    bool shadows = false;
    bool mirror = true;
};
struct Frame {
    std::vector<Pixel> pixels;
    uint64_t id = 0;
    uint64_t timestamp = 0;
};
struct Analysis {
    std::array<unsigned, 64> bins{};
    std::array<std::array<unsigned, 64>, 3> rgb{};
    std::array<unsigned, 64*32> waveform{};
    unsigned peak = 0;
    unsigned rgbPeak = 0;
    unsigned waveformPeak = 0;
    float highlights = 0;
    float shadows = 0;
};
struct Capture {
    Frame frame;
    Settings settings;
    std::string stem;
    std::string date;
    int fps = 30;
    float zoom = 1;
};
struct SaveResult {
    bool ok = false;
    bool originalSaved = false;
    std::string path;
    std::string error;
};
int stepEv(int value, int direction);
int cycleValue(int value, const int* values, size_t count, int direction = 1);
std::string evLabel(int value);
std::string wbLabel(int value);
std::string isoLabel(int value);
uint8_t luminance(Pixel pixel);
Pixel processPixel(Pixel pixel, Look look);
void processFrame(const std::vector<Pixel>& source, std::vector<Pixel>& target, Look look, bool mirror = false);
ImageCrop zoomCrop(float zoom, float centerX = Width/2.f, float centerY = Height/2.f);
void zoomFrame(const std::vector<Pixel>& source, std::vector<Pixel>& target, float zoom);
std::string zoomLabel(float zoom);
Analysis analyze(const std::vector<Pixel>& pixels, float zoom = 1);
std::string settingsText(const Settings& settings);
Settings parseSettings(const std::string& text);
std::string metadata(const Capture& capture);
bool writePng(const std::string& path, const std::vector<Pixel>& pixels, std::string& error);
SaveResult saveCapture(const std::string& directory, const Capture& capture);

class Countdown {
public:
    void start(uint64_t now, int seconds) { deadline_ = now + uint64_t(seconds) * 1000000; active_ = true; }
    void cancel() { active_ = false; }
    bool active() const { return active_; }
    bool due(uint64_t now) const { return active_ && now >= deadline_; }
    int remaining(uint64_t now) const { return !active_ || now >= deadline_ ? 0 : int((deadline_ - now + 999999) / 1000000); }
private:
    uint64_t deadline_ = 0;
    bool active_ = false;
};
}
