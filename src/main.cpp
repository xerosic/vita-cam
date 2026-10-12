#include "camera.hpp"
#include "storage.hpp"
#include "vita_painter.hpp"
#include <psp2/appmgr.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>
#include <psp2/rtc.h>
#include <psp2/touch.h>
#include <algorithm>
#include <cstdio>

extern "C" { unsigned int _newlib_heap_size_user = 64*1024*1024; }

namespace cam {
class App {
    VitaPainter painter;
    Camera camera;
    CaptureWriter writer{PhotoDirectory,commitPhoto};
    UiState ui;
    Frame frame, reviewFrame;
    Look reviewLook = Look::Original;
    Countdown timer;
    uint64_t serial = 0, toastUntil = 0, flashUntil = 0, lastStatus = 0, lastFrameTime = 0;
    uint64_t retryAt = 0, settleUntil = 0, saveSettingsAt = 0;
    uint64_t openedAt = 0;
    uint32_t previousButtons = 0;
    uint64_t repeatAt = 0;
    bool wasTouching = false;
    bool settingsDirty = false;
    bool previousCompare = false;
    bool liveDirty = true;
    bool readFrame = false;
    std::string reviewPath;
    SceTouchPanelInfo touchPanel{};
    std::vector<Hit> hits;
    uint64_t now() const { return sceKernelGetProcessTimeWide(); }
    void toast(const std::string& message, int seconds = 3) { ui.toast = message; toastUntil = now()+uint64_t(seconds)*1000000; }
    void changed() { settingsDirty = true; saveSettingsAt = now()+500000; liveDirty = true; }
    void openCamera() {
        timer.cancel();
        readFrame = false; ui.ready = false;
        if (!camera.open(ui.settings)) { ui.cameraError = camera.error(); retryAt = now()+3000000; }
        else {
            ui.cameraError.clear();
            if (!camera.error().empty()) toast(camera.error(),5);
            settleUntil = now()+500000;
        }
        ui.fps = camera.fps(); openedAt = now();
    }
    void set(CameraSetting setting, int value) {
        if (timer.active()) { toast("Cancel the timer before changing the camera"); return; }
        if (camera.set(setting,value,ui.settings)) { changed(); settleUntil = now()+300000; }
        else toast(camera.error(),5);
    }
    void back() {
        if (timer.active()) { timer.cancel(); return; }
        if (ui.screen != Screen::Camera) {
            ui.screen = Screen::Camera;
            updateReview(false);
            return;
        }
        ui.clean = !ui.clean; liveDirty = true;
    }
    void updateReview(bool original) {
        if (!reviewFrame.pixels.empty()) painter.uploadReview(reviewFrame,reviewLook,original);
        else if (!reviewPath.empty()) {
            std::string path = reviewPath;
            if (original) {
                for (size_t i = 1; i < LookNames.size(); ++i) {
                    const std::string suffix = std::string("-")+LookNames[i]+".png";
                    if (path.size() >= suffix.size() && path.compare(path.size()-suffix.size(),suffix.size(),suffix) == 0) {
                        path = path.substr(0,path.size()-suffix.size())+".png"; break;
                    }
                }
            }
            painter.loadReview(path);
        }
    }
    void shutter() {
        if (ui.screen == Screen::Review) { ui.zoom = !ui.zoom; return; }
        if (ui.screen != Screen::Camera) return;
        if (timer.active()) { timer.cancel(); return; }
        if (writer.failed()) { writer.retry(); toast("Retrying save"); return; }
        if (writer.busy()) { toast("Finishing the previous photograph"); return; }
        if (!ui.ready || !camera.ready()) { toast("Camera is unavailable"); return; }
        timer.start(now(),ui.settings.timer);
    }
    void capture(uint64_t time, bool fresh) {
        if (!timer.due(time) || !fresh || time < settleUntil) return;
        Capture shot;
        shot.frame = frame; shot.settings = ui.settings;
        shot.stem = captureName(++serial); shot.fps = camera.fps();
        if (!writer.submit(shot)) { timer.cancel(); toast("Capture queue is busy"); return; }
        reviewFrame = frame; reviewLook = ui.settings.look;
        painter.uploadReview(reviewFrame,reviewLook,false);
        ui.hasReview = true; ui.lastName = shot.stem;
        flashUntil = time+180000; timer.cancel();
    }
    void adjustRow(int row, int direction) {
        const auto& s = ui.settings;
        if (ui.tab == 0) {
            switch (row) {
            case 0: set(CameraSetting::WhiteBalance,cycleValue(s.wb,WhiteBalances.data(),WhiteBalances.size(),direction)); break;
            case 1: set(CameraSetting::Iso,cycleValue(s.iso,IsoModes.data(),IsoModes.size(),direction)); break;
            case 2: { const int values[]{1,2,3}; set(CameraSetting::Flicker,cycleValue(s.flicker,values,3,direction)); break; }
            case 3: ui.settings.look = Look((int(s.look)+direction+4)%4); changed(); break;
            case 4: ui.settings.timer = cycleValue(s.timer,Timers.data(),Timers.size(),direction); changed(); break;
            case 5: {
                timer.cancel();
                ui.settings.ev = 0; ui.settings.wb = 0; ui.settings.iso = 1; ui.settings.flicker = 1;
                ui.settings.look = Look::Original; ui.settings.timer = 0;
                openCamera(); changed(); toast("Camera defaults restored"); break;
            }
            case 6: ui.screen = Screen::Help; break;
            }
        } else {
            switch (row) {
            case 0: ui.settings.grid = Grid((int(s.grid)+direction+3)%3); break;
            case 1: ui.settings.histogram = !s.histogram; break;
            case 2: ui.settings.zebras = !s.zebras; break;
            case 3: ui.settings.shadows = !s.shadows; break;
            case 4: ui.settings.mirror = !s.mirror; break;
            case 5: ui.settings.grid = Grid::Thirds; ui.settings.histogram = true; ui.settings.zebras = false; ui.settings.shadows = false; ui.settings.mirror = true; break;
            }
            changed();
        }
    }
    void action(Hit hit) {
        switch (hit.action) {
        case Action::Shutter: shutter(); break;
        case Action::EvDown: set(CameraSetting::Ev,stepEv(ui.settings.ev,-1)); break;
        case Action::EvUp: set(CameraSetting::Ev,stepEv(ui.settings.ev,1)); break;
        case Action::Wb: set(CameraSetting::WhiteBalance,cycleValue(ui.settings.wb,WhiteBalances.data(),WhiteBalances.size())); break;
        case Action::Look: if (!timer.active()) { ui.settings.look = Look((int(ui.settings.look)+1)%4); changed(); } break;
        case Action::Timer: if (!timer.active()) { ui.settings.timer = cycleValue(ui.settings.timer,Timers.data(),Timers.size()); changed(); } break;
        case Action::SwitchCamera: ui.settings.device = 1-ui.settings.device; openCamera(); changed(); break;
        case Action::Review: timer.cancel(); ui.screen = Screen::Review; ui.zoom = false; ui.panX = 160; ui.panY = 120; updateReview(ui.compare); break;
        case Action::Settings: timer.cancel(); ui.screen = Screen::Settings; ui.row = 0; break;
        case Action::Back: case Action::Clean: back(); break;
        case Action::Tab: ui.tab = hit.value; ui.row = 0; break;
        case Action::Row: ui.row = hit.value; adjustRow(hit.value,1); break;
        case Action::Zoom: ui.zoom = !ui.zoom; break;
        case Action::Help: timer.cancel(); ui.screen = Screen::Help; break;
        default: break;
        }
    }
    void buttonAction(Action a) { action({{0,0,0,0},a}); }
    void input(uint64_t time) {
        SceCtrlData pad{};
        sceCtrlPeekBufferPositive(0,&pad,1);
        uint32_t pressed = pad.buttons & ~previousButtons;
        const uint32_t arrows = SCE_CTRL_LEFT|SCE_CTRL_RIGHT|SCE_CTRL_UP|SCE_CTRL_DOWN;
        if (pressed & arrows) repeatAt = time+350000;
        else if ((pad.buttons & arrows) && time >= repeatAt) { pressed |= pad.buttons & arrows; repeatAt = time+110000; }
        previousButtons = pad.buttons;
        ui.compare = (pad.buttons & SCE_CTRL_SELECT) != 0;
        if (ui.compare != previousCompare) {
            liveDirty = true;
            if (ui.screen == Screen::Review) updateReview(ui.compare);
            previousCompare = ui.compare;
        }
        if (pressed & SCE_CTRL_CIRCLE) buttonAction(Action::Back);
        else if (pressed & SCE_CTRL_START) buttonAction(Action::Help);
        else if (ui.screen == Screen::Settings) {
            if (pressed & SCE_CTRL_UP) ui.row = (ui.row+settingsRows(ui.tab)-1)%settingsRows(ui.tab);
            if (pressed & SCE_CTRL_DOWN) ui.row = (ui.row+1)%settingsRows(ui.tab);
            if (pressed & SCE_CTRL_LEFT) adjustRow(ui.row,-1);
            if (pressed & (SCE_CTRL_RIGHT|SCE_CTRL_CROSS)) adjustRow(ui.row,1);
            if (pressed & (SCE_CTRL_LTRIGGER|SCE_CTRL_RTRIGGER)) { ui.tab = 1-ui.tab; ui.row = 0; }
            if (pressed & SCE_CTRL_TRIANGLE) back();
        } else if (ui.screen == Screen::Review) {
            if (pressed & (SCE_CTRL_CROSS|SCE_CTRL_RTRIGGER)) ui.zoom = !ui.zoom;
            if (pressed & SCE_CTRL_SQUARE) back();
            if (ui.zoom) {
                if (pressed & SCE_CTRL_LEFT) ui.panX -= 16;
                if (pressed & SCE_CTRL_RIGHT) ui.panX += 16;
                if (pressed & SCE_CTRL_UP) ui.panY -= 16;
                if (pressed & SCE_CTRL_DOWN) ui.panY += 16;
                if (pad.lx < 88 || pad.lx > 168) ui.panX += (int(pad.lx)-128)/24.f;
                if (pad.ly < 88 || pad.ly > 168) ui.panY += (int(pad.ly)-128)/24.f;
                ui.panX = std::clamp(ui.panX,0.f,320.f); ui.panY = std::clamp(ui.panY,0.f,240.f);
            }
        } else if (ui.screen == Screen::Camera) {
            if (pressed & (SCE_CTRL_RTRIGGER|SCE_CTRL_CROSS)) buttonAction(Action::Shutter);
            if (pressed & SCE_CTRL_LTRIGGER) buttonAction(Action::SwitchCamera);
            if (pressed & SCE_CTRL_LEFT) buttonAction(Action::EvDown);
            if (pressed & SCE_CTRL_RIGHT) buttonAction(Action::EvUp);
            if (pressed & SCE_CTRL_UP) buttonAction(Action::Wb);
            if (pressed & SCE_CTRL_DOWN) buttonAction(Action::Look);
            if (pressed & SCE_CTRL_SQUARE) buttonAction(Action::Review);
            if (pressed & SCE_CTRL_TRIANGLE) buttonAction(Action::Settings);
        }
        SceTouchData touch{};
        sceTouchPeek(SCE_TOUCH_PORT_FRONT,&touch,1);
        if (touch.reportNum && !wasTouching) {
            const auto& report = touch.report[0];
            const float x = (int(report.x)-touchPanel.minAaX)*960.f/(touchPanel.maxAaX-touchPanel.minAaX+1);
            const float y = (int(report.y)-touchPanel.minAaY)*544.f/(touchPanel.maxAaY-touchPanel.minAaY+1);
            action(hitTest(hits,x,y));
        }
        wasTouching = touch.reportNum != 0;
    }
public:
    int run() {
        if (!painter.valid()) return 1;
        const bool storage = prepareStorage();
        ui.settings = loadSettings();
        reviewPath = lastPhoto();
        if (!reviewPath.empty()) ui.hasReview = painter.loadReview(reviewPath);
        sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
        sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,SCE_TOUCH_SAMPLING_STATE_START);
        if (sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT,&touchPanel) < 0) {
            touchPanel.minAaX = 0; touchPanel.maxAaX = 1919; touchPanel.minAaY = 0; touchPanel.maxAaY = 1087;
        }
        openCamera();
        if (!storage) toast("Storage unavailable; check ux0:",8);
        uint64_t previousTime = now();
        while (true) {
            const uint64_t time = now();
            SceAppMgrSystemEvent event{};
            const bool resumed = sceAppMgrReceiveSystemEvent(&event) >= 0 && event.systemEvent == SCE_APPMGR_SYSTEMEVENT_ON_RESUME;
            if (resumed || time-previousTime > 2000000) { openCamera(); previousButtons = 0; }
            previousTime = time;
            input(time);
            bool fresh = false;
            if (camera.ready()) {
                fresh = camera.read(frame);
                if (fresh) { lastFrameTime = time; readFrame = true; liveDirty = true; }
                else if (!camera.ready()) {
                    ui.cameraError = camera.error(); readFrame = false;
                    retryAt = time+3000000; timer.cancel();
                }
            } else if (time >= retryAt) openCamera();
            if (camera.ready() && !fresh && time-(readFrame ? lastFrameTime : openedAt) > 2000000) {
                camera.close(); readFrame = false; timer.cancel();
                ui.cameraError = "Camera stopped delivering frames"; retryAt = time+1000000;
            }
            ui.ready = camera.ready() && readFrame && time-lastFrameTime < 1000000;
            if (!ui.ready && timer.active() && time-lastFrameTime > 1500000) { timer.cancel(); toast("Camera stalled; capture cancelled"); }
            capture(time,fresh);
            if (liveDirty && readFrame) {
                painter.uploadLive(frame,ui);
                ui.analysis = analyze(painter.livePixels());
                liveDirty = false;
            }
            if (auto result = writer.poll()) {
                if (result->ok) { ++ui.saved; reviewPath = result->path; toast("Photo saved"); }
                else {
                    toast(result->originalSaved ? "Original kept. Press Capture to retry remaining files." : "Save failed. Frame kept; press Capture to retry.",8);
                    logResult(result->error.c_str(),-1);
                }
            }
            ui.failedSave = writer.failed(); ui.saving = writer.busy();
            if (time >= toastUntil) ui.toast.clear();
            ui.flash = time < flashUntil ? float(flashUntil-time)/180000.f : 0;
            ui.countdown = timer.remaining(time);
            if (settingsDirty && time >= saveSettingsAt) {
                if (!storeSettings(ui.settings)) toast("Could not save preferences");
                settingsDirty = false;
            }
            if (time-lastStatus >= 1000000) {
                SceDateTime date{}; sceRtcGetCurrentClockLocalTime(&date);
                char clock[12]; std::snprintf(clock,sizeof(clock),"%02u:%02u",date.hour,date.minute); ui.clock = clock;
                ui.battery = scePowerGetBatteryLifePercent();
                lastStatus = time;
            }
            if (timer.active() || ui.saving) sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
            painter.begin(); hits = drawUi(painter,ui); painter.end();
            if (!camera.ready()) sceKernelDelayThread(16000);
        }
    }
};
}
int main() {
    int result = 1;
    { cam::App app; result = app.run(); }
    sceKernelExitProcess(result);
    return result;
}
