#pragma once
#include "core.hpp"
#include <psp2/types.h>

namespace cam {
enum class CameraSetting { Ev, WhiteBalance, Iso, Flicker };
class Camera {
public:
    Camera() = default;
    ~Camera();
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    bool open(Settings& requested);
    void close();
    bool read(Frame& frame);
    bool set(CameraSetting setting, int value, Settings& requested);
    bool cycle(CameraSetting setting, const int* values, size_t count, int direction, Settings& requested);
    bool ready() const { return started_; }
    int fps() const { return fps_; }
    const std::string& error() const { return error_; }

private:
    bool result(const char* operation, int code);
    void readSettings(Settings& requested);
    SceUID memory_ = -1;
    void* buffer_ = nullptr;
    bool opened_ = false;
    bool started_ = false;
    bool hasFrame_ = false;
    int device_ = 1;
    int fps_ = 30;
    uint64_t lastFrame_ = 0;
    int readErrors_ = 0;
    bool rejectedValue_ = false;
    std::string error_;
};
void logResult(const char* operation, int code, int value = 0);
}
