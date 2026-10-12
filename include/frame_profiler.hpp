#pragma once
#include "ui.hpp"
#ifdef VITACAM_PROFILE
#include "camera.hpp"
#include <psp2/kernel/processmgr.h>
#include <algorithm>
#include <cstdio>
#endif

namespace cam {
class FrameProfiler {
public:
    enum Stage { CameraRead, Preview, Analysis, Other, Draw, Count };
#ifdef VITACAM_PROFILE
    void reset() { start_ = 0; }
    void begin(const UiState& ui) {
        const unsigned mode = unsigned(ui.screen)|(unsigned(ui.settings.graph)<<3)|(unsigned(ui.settings.look)<<6)
            |(ui.settings.zebras<<9)|(ui.settings.shadows<<10)|(ui.clean<<11)|(ui.compare<<12)|(ui.settings.device<<13);
        const auto time = sceKernelGetProcessTimeWide();
        if (mode != mode_ || !start_) {
            mode_ = mode; start_ = previous_ = time; frames_ = fresh_ = 0; longest_ = 0; totals_.fill(0);
        }
        longest_ = std::max(longest_,time-previous_);
        previous_ = mark_ = time;
    }
    void mark(Stage stage) {
        const auto time = sceKernelGetProcessTimeWide();
        totals_[stage] += time-mark_; mark_ = time;
    }
    void end(const UiState& ui,bool fresh) {
        ++frames_; fresh_ += fresh;
        const auto elapsed = mark_-start_;
        if (elapsed < 5000000 || ui.screen != Screen::Camera) return;
        char line[320];
        std::snprintf(line,sizeof(line),
            "perf graph=%s look=%s zebras=%d shadows=%d clean=%d ui_fps=%.1f camera_fps=%.1f max_interval_ms=%.1f mean_us read=%llu preview=%llu analysis=%llu other=%llu draw_wait=%llu",
            GraphNames[int(ui.settings.graph)],LookNames[int(ui.settings.look)],ui.settings.zebras,ui.settings.shadows,ui.clean,
            frames_*1000000.0/elapsed,fresh_*1000000.0/elapsed,longest_/1000.0,
            static_cast<unsigned long long>(totals_[CameraRead]/frames_),static_cast<unsigned long long>(totals_[Preview]/frames_),
            static_cast<unsigned long long>(totals_[Analysis]/frames_),static_cast<unsigned long long>(totals_[Other]/frames_),
            static_cast<unsigned long long>(totals_[Draw]/frames_));
        logResult(line,0);
        start_ = 0;
    }
private:
    unsigned mode_ = 0, frames_ = 0, fresh_ = 0;
    uint64_t start_ = 0, previous_ = 0, mark_ = 0, longest_ = 0;
    std::array<uint64_t,Count> totals_{};
#else
    void reset() {}
    void begin(const UiState&) {}
    void mark(Stage) {}
    void end(const UiState&,bool) {}
#endif
};
}
