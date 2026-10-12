#include "camera.hpp"
#include "gallery.hpp"
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
    PhotoShare sharing;
    Screen shareReturn = Screen::Camera;
    UiState ui;
    Frame frame, reviewFrame;
    Look reviewLook = Look::Original;
    float captureZoom = 1;
    Countdown timer;
    uint64_t serial = 0, toastUntil = 0, flashUntil = 0, lastStatus = 0, lastFrameTime = 0;
    uint64_t retryAt = 0, settleUntil = 0, saveSettingsAt = 0;
    uint64_t openedAt = 0;
    uint64_t lastInput = 0;
    uint32_t previousButtons = 0;
    uint64_t repeatAt = 0;
    bool wasTouching = false;
    float touchStartX = 0, touchStartY = 0, touchLastX = 0, touchLastY = 0;
    bool settingsDirty = false;
    bool previousCompare = false;
    bool liveDirty = true;
    bool readFrame = false;
    std::string reviewPath;
    std::vector<PhotoEntry> gallery;
    size_t galleryIndex = 0;
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
    void cycle(CameraSetting setting,const int* values,size_t count,int direction = 1) {
        if (timer.active()) { toast("Cancel the timer before changing the camera"); return; }
        if (camera.cycle(setting,values,count,direction,ui.settings)) { changed(); settleUntil = now()+300000; }
        else toast(camera.error(),3);
    }
    void back() {
        if (timer.active()) { timer.cancel(); return; }
        if (ui.screen == Screen::Share) {
            sharing.stop(); ui.sharing = false; ui.qr = {};
            ui.screen = shareReturn; openCamera(); updateReview(ui.screen == Screen::Review && ui.compare); return;
        }
        if (ui.screen != Screen::Camera) {
            ui.screen = Screen::Camera;
            updateReview(false);
            return;
        }
        ui.clean = !ui.clean; liveDirty = true;
    }
    void share() {
        if (writer.busy()) { toast("Wait for the photo to finish saving"); return; }
        timer.cancel();
        if (ui.screen != Screen::Share) shareReturn = ui.screen;
        ui.screen = Screen::Share; ui.toast.clear();
        camera.close(); readFrame = false; ui.ready = false;
        refreshGallery();
        ui.sharing = sharing.start(PhotoDirectory,ui.shareError);
        ui.shareUrl = sharing.url(); ui.qr = ui.sharing ? makeQr(ui.shareUrl) : QrCode{};
    }
    void refreshGallery(bool newest = false) {
        const std::string selected = !newest && galleryIndex < gallery.size() ? gallery[galleryIndex].original : "";
        gallery = scanPhotos(PhotoDirectory);
        galleryIndex = 0;
        for (size_t i = 0; i < gallery.size(); ++i) if (gallery[i].original == selected) galleryIndex = i;
        ui.photoCount = int(gallery.size());
        ui.photoIndex = gallery.empty() ? 0 : int(galleryIndex)+1;
    }
    void browse(int direction) {
        if (gallery.empty()) return;
        const int index = std::clamp(int(galleryIndex)+direction,0,int(gallery.size())-1);
        if (size_t(index) == galleryIndex) return;
        galleryIndex = size_t(index); ui.photoIndex = index+1;
        ui.reviewZoom = 1; ui.panX = Width/2.f; ui.panY = Height/2.f;
        updateReview(ui.compare);
    }
    void updateReview(bool original) {
        if (ui.screen == Screen::Review) {
            ui.reviewReady = false;
            if (galleryIndex >= gallery.size()) return;
            const auto& photo = gallery[galleryIndex];
            ui.lastName = photoLabel(photo.original);
            ui.reviewReady = painter.loadReview(std::string(PhotoDirectory)+"/"+(original ? photo.original : photo.image));
            if (!ui.reviewReady) toast("Could not open this photo");
        } else if (!reviewFrame.pixels.empty()) {
            painter.uploadReview(reviewFrame,reviewLook,false,captureZoom);
        } else if (!reviewPath.empty()) {
            ui.hasReview = painter.loadReview(reviewPath);
        }
    }
    void shutter() {
        if (ui.screen == Screen::Review) { ui.reviewZoom = ui.reviewZoom > 1 ? 1 : 2; return; }
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
        shot.zoom = ui.zoom;
        shot.stem = captureName(++serial); shot.fps = camera.fps();
        if (!writer.submit(shot)) { timer.cancel(); toast("Capture queue is busy"); return; }
        reviewFrame = frame; reviewLook = ui.settings.look; captureZoom = ui.zoom;
        painter.uploadReview(reviewFrame,reviewLook,false,captureZoom);
        ui.hasReview = true; ui.lastName = shot.stem;
        flashUntil = time+180000; timer.cancel();
    }
    void adjustRow(int row, int direction) {
        const auto& s = ui.settings;
        if (ui.tab == 0) {
            switch (row) {
            case 0: cycle(CameraSetting::WhiteBalance,WhiteBalances.data(),WhiteBalances.size(),direction); break;
            case 1: cycle(CameraSetting::Iso,IsoModes.data(),IsoModes.size(),direction); break;
            case 2: { const int values[]{1,2,3}; cycle(CameraSetting::Flicker,values,3,direction); break; }
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
            case 1: ui.settings.graph = Graph((int(s.graph)+direction+4)%4); break;
            case 2: ui.settings.zebras = !s.zebras; break;
            case 3: ui.settings.shadows = !s.shadows; break;
            case 4: ui.settings.mirror = !s.mirror; break;
            case 5: ui.settings.grid = Grid::Thirds; ui.settings.graph = Graph::Luminance; ui.settings.zebras = false; ui.settings.shadows = false; ui.settings.mirror = true; break;
            }
            changed();
        }
    }
    void action(Hit hit) {
        switch (hit.action) {
        case Action::Shutter: shutter(); break;
        case Action::EvDown: set(CameraSetting::Ev,stepEv(ui.settings.ev,-1)); break;
        case Action::EvUp: set(CameraSetting::Ev,stepEv(ui.settings.ev,1)); break;
        case Action::Wb: cycle(CameraSetting::WhiteBalance,WhiteBalances.data(),WhiteBalances.size()); break;
        case Action::Look: if (!timer.active()) { ui.settings.look = Look((int(ui.settings.look)+1)%4); changed(); } break;
        case Action::GraphNext: case Action::GraphPrevious:
            ui.settings.graph = Graph((int(ui.settings.graph)+(hit.action == Action::GraphNext ? 1 : 3))%4); changed(); break;
        case Action::Timer: if (!timer.active()) { ui.settings.timer = cycleValue(ui.settings.timer,Timers.data(),Timers.size()); changed(); } break;
        case Action::SwitchCamera: ui.settings.device = 1-ui.settings.device; openCamera(); changed(); break;
        case Action::Review:
            timer.cancel(); refreshGallery(true); ui.screen = Screen::Review;
            ui.reviewZoom = 1; ui.panX = Width/2.f; ui.panY = Height/2.f; updateReview(ui.compare); break;
        case Action::PreviousPhoto: browse(-1); break;
        case Action::NextPhoto: browse(1); break;
        case Action::Share: share(); break;
        case Action::Settings: timer.cancel(); ui.screen = Screen::Settings; ui.row = 0; break;
        case Action::Back: case Action::Clean: back(); break;
        case Action::Tab: ui.tab = hit.value; ui.row = 0; break;
        case Action::Row: ui.row = hit.value; adjustRow(hit.value,1); break;
        case Action::Zoom:
            if (ui.screen == Screen::Review) ui.reviewZoom = ui.reviewZoom > 1 ? 1 : 2;
            else if (!timer.active()) { ui.zoom = 1; liveDirty = true; }
            break;
        case Action::Help: timer.cancel(); ui.screen = Screen::Help; break;
        default: break;
        }
    }
    void buttonAction(Action a) { action({{0,0,0,0},a}); }
    void input(uint64_t time) {
        const float dt = lastInput ? std::min(.05f,(time-lastInput)/1000000.f) : 0;
        lastInput = time;
        SceCtrlData pad{};
        sceCtrlPeekBufferPositive(0,&pad,1);
        const auto axis = [](int value) { const int offset = value-128; return std::abs(offset) <= 24 ? 0.f : (offset > 0 ? offset-24 : offset+24)/103.f; };
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
        if (ui.screen == Screen::Share) {
            if (pressed & SCE_CTRL_CIRCLE) back();
            else if ((pressed & SCE_CTRL_CROSS) && !ui.sharing) share();
        } else if (pressed & SCE_CTRL_CIRCLE) buttonAction(Action::Back);
        else if (pressed & SCE_CTRL_START) buttonAction(Action::Help);
        else if (ui.screen == Screen::Settings) {
            if (pressed & SCE_CTRL_UP) ui.row = (ui.row+settingsRows(ui.tab)-1)%settingsRows(ui.tab);
            if (pressed & SCE_CTRL_DOWN) ui.row = (ui.row+1)%settingsRows(ui.tab);
            if (pressed & SCE_CTRL_LEFT) adjustRow(ui.row,-1);
            if (pressed & (SCE_CTRL_RIGHT|SCE_CTRL_CROSS)) adjustRow(ui.row,1);
            if (pressed & (SCE_CTRL_LTRIGGER|SCE_CTRL_RTRIGGER)) { ui.tab = 1-ui.tab; ui.row = 0; }
            if (pressed & SCE_CTRL_TRIANGLE) back();
        } else if (ui.screen == Screen::Review) {
            if (pressed & (SCE_CTRL_CROSS|SCE_CTRL_RTRIGGER)) buttonAction(Action::Zoom);
            if (pressed & SCE_CTRL_SQUARE) back();
            if (pressed & SCE_CTRL_LEFT) browse(-1);
            if (pressed & SCE_CTRL_RIGHT) browse(1);
            ui.reviewZoom = std::clamp(ui.reviewZoom-axis(pad.ry)*dt*1.5f,1.f,MaxZoom);
            if (ui.reviewZoom > 1) {
                ui.panX += axis(pad.lx)*dt*240; ui.panY += axis(pad.ly)*dt*240;
            }
            const auto crop = zoomCrop(ui.reviewZoom,ui.panX,ui.panY);
            ui.panX = crop.x+crop.w/2; ui.panY = crop.y+crop.h/2;
        } else if (ui.screen == Screen::Camera) {
            if (pressed & (SCE_CTRL_RTRIGGER|SCE_CTRL_CROSS)) buttonAction(Action::Shutter);
            if (pressed & SCE_CTRL_LTRIGGER) buttonAction(Action::SwitchCamera);
            if (pressed & SCE_CTRL_LEFT) buttonAction(Action::EvDown);
            if (pressed & SCE_CTRL_RIGHT) buttonAction(Action::EvUp);
            if (pressed & SCE_CTRL_UP) buttonAction(Action::GraphPrevious);
            if (pressed & SCE_CTRL_DOWN) buttonAction(Action::GraphNext);
            if (!timer.active() && axis(pad.ry) != 0) { ui.zoom = std::clamp(ui.zoom-axis(pad.ry)*dt*1.5f,1.f,MaxZoom); liveDirty = true; }
            if (pressed & SCE_CTRL_SQUARE) buttonAction(Action::Review);
            if (pressed & SCE_CTRL_TRIANGLE) buttonAction(Action::Settings);
        }
        SceTouchData touch{};
        sceTouchPeek(SCE_TOUCH_PORT_FRONT,&touch,1);
        if (touch.reportNum) {
            const auto& report = touch.report[0];
            const float x = (int(report.x)-touchPanel.minAaX)*960.f/(touchPanel.maxAaX-touchPanel.minAaX+1);
            const float y = (int(report.y)-touchPanel.minAaY)*544.f/(touchPanel.maxAaY-touchPanel.minAaY+1);
            touchLastX = x; touchLastY = y;
            if (!wasTouching) { touchStartX = x; touchStartY = y; action(hitTest(hits,x,y)); }
        } else if (wasTouching && ui.screen == Screen::Review && Viewport.contains(touchStartX,touchStartY)) {
            const float dx = touchLastX-touchStartX, dy = touchLastY-touchStartY;
            if (std::abs(dx) > 60 && std::abs(dx) > std::abs(dy)) browse(dx > 0 ? -1 : 1);
        }
        wasTouching = touch.reportNum != 0;
    }
public:
    int run(bool storage) {
        if (!painter.valid()) return 1;
        logResult("startup: renderer and capture worker ready",0);
        ui.settings = loadSettings();
        reviewPath = lastPhoto();
        if (!reviewPath.empty()) ui.hasReview = painter.loadReview(reviewPath);
        refreshGallery(true);
        if (!ui.hasReview && !gallery.empty()) { reviewPath = std::string(PhotoDirectory)+"/"+gallery.front().image; ui.hasReview = painter.loadReview(reviewPath); }
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
            if (resumed || time-previousTime > 2000000) {
                if (ui.screen == Screen::Share) {
                    sharing.stop(); ui.sharing = false; ui.qr = {}; ui.shareError = "Sharing paused. Select Retry to reconnect.";
                } else openCamera();
                previousButtons = 0;
            }
            previousTime = time;
            input(time);
            if (ui.screen == Screen::Share) {
                if (ui.sharing && time-lastStatus >= 1000000) {
                    lastStatus = time;
                    if (!sharing.connected()) { sharing.stop(); ui.sharing = false; ui.qr = {}; ui.shareError = "Wi-Fi disconnected. Reconnect, then retry."; }
                }
                sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
                painter.begin(); hits = drawUi(painter,ui); painter.end();
                sceKernelDelayThread(16000);
                continue;
            }
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
                ui.analysis = analyze(painter.livePixels(),ui.zoom);
                liveDirty = false;
            }
            if (auto result = writer.poll()) {
                if (result->ok) {
                    ++ui.saved; reviewPath = result->path; toast("Photo saved");
                    refreshGallery(ui.screen != Screen::Review);
                    if (ui.screen == Screen::Review) updateReview(ui.compare);
                }
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
    const bool storage = cam::prepareStorage();
    cam::logResult("startup: constructing application",0);
    int result = 1;
    { cam::App app; result = app.run(storage); }
    sceKernelExitProcess(result);
    return result;
}
