#include "camera.hpp"
#include <psp2/camera.h>
#include <psp2/kernel/sysmem.h>
#include <cstdio>
#include <cstring>

namespace cam {
void logResult(const char* operation, int code, int value) {
    if (FILE* f = std::fopen("ux0:data/VitaCamera/session.log", "a")) {
        std::fprintf(f, "%s value=%d result=0x%08x\n", operation, value, unsigned(code));
        std::fclose(f);
    }
}
Camera::~Camera() { close(); }
bool Camera::result(const char* operation, int code) {
    logResult(operation, code);
    if (code >= 0) return true;
    char message[96];
    std::snprintf(message, sizeof(message), "%s: 0x%08x", operation, unsigned(code));
    error_ = message;
    return false;
}
void Camera::close() {
    if (started_) logResult("stop", sceCameraStop(device_));
    if (opened_) logResult("close", sceCameraClose(device_));
    if (memory_ >= 0) sceKernelFreeMemBlock(memory_);
    memory_ = -1; buffer_ = nullptr; opened_ = started_ = hasFrame_ = false;
}
bool Camera::open(Settings& requested) {
    close();
    error_.clear(); readErrors_ = 0;
    device_ = requested.device;
    constexpr unsigned alignment = 256*1024;
    constexpr unsigned bytes = (Width*Height*sizeof(Pixel)+alignment-1)/alignment*alignment;
    memory_ = sceKernelAllocMemBlock("VitaCameraFrame", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, bytes, nullptr);
    if (!result("Camera memory", memory_)) return false;
    if (!result("Camera buffer", sceKernelGetMemBlockBase(memory_, &buffer_))) { close(); return false; }
    std::memset(buffer_, 0, bytes);
    for (int fps : {60, 30}) {
        SceCameraInfo info{};
        info.size = sizeof(info);
        info.format = SCE_CAMERA_FORMAT_ABGR;
        info.resolution = SCE_CAMERA_RESOLUTION_640_480;
        info.framerate = fps;
        info.sizeIBase = Width*Height*sizeof(Pixel);
        info.pIBase = buffer_;
        const int code = sceCameraOpen(device_, &info);
        logResult("open", code, fps);
        if (code >= 0) {
            opened_ = true; fps_ = fps;
            if (result("Camera start", sceCameraStart(device_))) { started_ = true; break; }
            logResult("close after failed start",sceCameraClose(device_));
            opened_ = false;
        } else result("Camera open", code);
    }
    if (!started_) { close(); return false; }
    // Read defaults first, so a rejected request cannot become a displayed setting.
    const Settings desired = requested;
    readSettings(requested);
    std::string rejected;
    for (const auto& pair : {std::pair<CameraSetting,int>{CameraSetting::Ev, desired.ev},
            {CameraSetting::WhiteBalance, desired.wb}, {CameraSetting::Iso, desired.iso},
            {CameraSetting::Flicker, desired.flicker}}) {
        if (!set(pair.first, pair.second, requested)) rejected = error_;
    }
    // Reopen preserves the ISP's processing defaults; only orientation is explicit.
    if (!result("Camera orientation", sceCameraSetReverse(device_, SCE_CAMERA_REVERSE_OFF))) rejected = error_;
    error_ = rejected;
    return true;
}
void Camera::readSettings(Settings& s) {
    struct Entry { const char* name; int (*get)(int,int*); int* destination; };
    for (auto e : {Entry{"get EV",sceCameraGetEV,&s.ev}, {"get WB",sceCameraGetWhiteBalance,&s.wb},
            {"get ISO",sceCameraGetISO,&s.iso}, {"get flicker",sceCameraGetAntiFlicker,&s.flicker}}) {
        int value = *e.destination;
        const int code = e.get(device_, &value);
        logResult(e.name,code,value);
        if (code >= 0) *e.destination = value;
    }
}
bool Camera::set(CameraSetting setting, int value, Settings& requested) {
    if (!started_) { error_ = "Camera is unavailable"; return false; }
    const char* name = "";
    int code = -1;
    int* destination = nullptr;
    int (*getter)(int,int*) = nullptr;
    switch (setting) {
    case CameraSetting::Ev: name = "EV"; code = sceCameraSetEV(device_,value); destination = &requested.ev; getter = sceCameraGetEV; break;
    case CameraSetting::WhiteBalance: name = "White balance"; code = sceCameraSetWhiteBalance(device_,value); destination = &requested.wb; getter = sceCameraGetWhiteBalance; break;
    case CameraSetting::Iso: name = "ISO"; code = sceCameraSetISO(device_,value); destination = &requested.iso; getter = sceCameraGetISO; break;
    case CameraSetting::Flicker: name = "Anti-flicker"; code = sceCameraSetAntiFlicker(device_,value); destination = &requested.flicker; getter = sceCameraGetAntiFlicker; break;
    }
    logResult(name,code,value);
    if (!result(name,code)) return false;
    *destination = value;
    int returned = value;
    const int got = getter(device_,&returned);
    logResult("readback",got,returned);
    error_.clear();
    return true;
}
bool Camera::read(Frame& frame) {
    if (!started_) return false;
    SceCameraRead read{};
    read.size = sizeof(read);
    read.mode = 0;
    const int code = sceCameraRead(device_, &read);
    if (code < 0) {
        if (uint32_t(code) == SCE_CAMERA_ERROR_ALREADY_READ) return false;
        if (++readErrors_ == 1) result("Camera read", code);
        if (readErrors_ >= 3) close();
        return false;
    }
    readErrors_ = 0;
    if (hasFrame_ && read.frame == lastFrame_) return false;
    lastFrame_ = read.frame; hasFrame_ = true;
    frame.pixels.resize(Width*Height);
    // The next camera read cannot replace this app-owned frame or queued captures.
    std::memcpy(frame.pixels.data(), buffer_, Width*Height*sizeof(Pixel));
    frame.id = read.frame; frame.timestamp = read.timestamp;
    return true;
}
}
