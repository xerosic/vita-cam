#pragma once
#include "core.hpp"

namespace cam {
using Color = uint32_t;
constexpr Color rgba(unsigned r, unsigned g, unsigned b, unsigned a = 255) { return r | g<<8 | b<<16 | a<<24; }
struct Rect {
    float x, y, w, h;
    bool contains(float px, float py) const { return px >= x && py >= y && px < x+w && py < y+h; }
};
enum class Action { None, Shutter, EvDown, EvUp, Wb, Look, Timer, SwitchCamera, Review, Settings, Clean, Back, Tab, Row, Zoom, Help };
struct Hit { Rect rect; Action action; int value = 0; };
enum class Screen { Camera, Settings, Review, Help };
struct UiState {
    Settings settings;
    Screen screen = Screen::Camera;
    Analysis analysis;
    bool ready = false;
    bool hasReview = false;
    bool clean = false;
    bool compare = false;
    bool zoom = false;
    bool saving = false;
    bool failedSave = false;
    int countdown = 0;
    int tab = 0;
    int row = 0;
    int battery = -1;
    int saved = 0;
    int fps = 30;
    float panX = 160, panY = 120;
    float flash = 0;
    std::string clock = "--:--";
    std::string freeSpace;
    std::string toast;
    std::string cameraError;
    std::string lastName;
};
class Painter {
public:
    virtual ~Painter() = default;
    virtual void rect(Rect r, Color color) = 0;
    virtual void circle(float x, float y, float radius, Color color) = 0;
    virtual void line(float x, float y, float x2, float y2, Color color) = 0;
    virtual void text(float x, float baseline, float size, Color color, const std::string& text) = 0;
    virtual void image(int id, Rect destination, Rect source) = 0;
    void round(Rect r, float radius, Color color);
    void gradient(Rect r, Color top, Color bottom);
};
constexpr Rect Viewport{144, 40, 640, 480};
constexpr Rect FullViewport{117.333f, 0, 725.333f, 544};
int settingsRows(int tab);
std::string settingName(int tab, int row);
std::string settingValue(const UiState& state, int row);
std::vector<Hit> drawUi(Painter& p, const UiState& state);
Hit hitTest(const std::vector<Hit>& hits, float x, float y);
}
