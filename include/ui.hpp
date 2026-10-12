#pragma once
#include "core.hpp"
#include "sharing.hpp"
#include "text_layout.hpp"

namespace cam {
using Color = uint32_t;
constexpr Color rgba(unsigned r, unsigned g, unsigned b, unsigned a = 255) { return r | g << 8 | b << 16 | a << 24; }
struct Rect {
    float x, y, w, h;
    bool contains(float px, float py) const { return px >= x && py >= y && px < x + w && py < y + h; }
};
enum class Action {
    None,
    Shutter,
    EvDown,
    EvUp,
    Wb,
    Look,
    GraphNext,
    GraphPrevious,
    Timer,
    SwitchCamera,
    Review,
    PreviousPhoto,
    NextPhoto,
    Share,
    Settings,
    Clean,
    Back,
    Tab,
    Row,
    Zoom,
    Help
};
struct Hit {
    Rect rect;
    Action action;
    int value = 0;
};
enum class Screen { Camera, Settings, Review, Help, Share };
class CleanViewHold {
public:
    bool update(Screen screen, bool circleDown, bool canStartCircle, bool viewfinderHeld);

private:
    bool previousCircle_ = false;
    bool circleActive_ = false;
};
struct UiState {
    Settings settings;
    Screen screen = Screen::Camera;
    Analysis analysis;
    bool ready = false;
    bool hasReview = false;
    bool reviewReady = false;
    bool clean = false;
    bool compare = false;
    bool saving = false;
    bool failedSave = false;
    bool sharing = false;
    int countdown = 0;
    int tab = 0;
    int row = 0;
    int battery = -1;
    int saved = 0;
    int fps = 30;
    int photoIndex = 0, photoCount = 0;
    float zoom = 1, reviewZoom = 1;
    float panX = Width / 2.f, panY = Height / 2.f;
    float flash = 0;
    std::string clock = "--:--";
    std::string freeSpace;
    std::string toast;
    std::string cameraError;
    std::string lastName;
    std::string shareUrl, shareError;
    QrCode qr;
};
class Painter {
public:
    virtual ~Painter() = default;
    virtual void rect(Rect r, Color color) = 0;
    virtual void circle(float x, float y, float radius, Color color) = 0;
    virtual void ring(float x, float y, float radius, Color color) = 0;
    virtual void round(Rect r, float radius, Color color) = 0;
    virtual void line(float x, float y, float x2, float y2, Color color) = 0;
    virtual void text(float x, float baseline, float size, Color color, const std::string& text) = 0;
    virtual TextBounds textBounds(float size, const std::string& text) = 0;
    virtual void image(int id, Rect destination, Rect source) = 0;
    virtual void gradient(Rect r, Color top, Color bottom);
    void centeredText(Rect r, float size, Color color, const std::string& text);
};
constexpr Rect Viewport{144, 40, 640, 480};
constexpr Rect FullViewport{117.333f, 0, 725.333f, 544};
int settingsRows(int tab);
std::string settingName(int tab, int row);
std::string settingValue(const UiState& state, int row);
std::vector<Hit> drawUi(Painter& p, const UiState& state);
Hit hitTest(const std::vector<Hit>& hits, float x, float y);
}
