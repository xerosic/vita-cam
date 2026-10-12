#pragma once
#include "core.hpp"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace cam {
constexpr const char* DataDirectory = "ux0:data/VitaCamera";
constexpr const char* PhotoDirectory = "ux0:data/VitaCamera/photos";
bool prepareStorage();
Settings loadSettings();
bool storeSettings(const Settings& settings);
std::string captureName(uint64_t serial);
std::string lastPhoto();
bool storeLastPhoto(const std::string& path);
void commitPhoto(const std::string& path);
class CaptureWriter {
public:
    explicit CaptureWriter(std::string directory, void (*onSaved)(const std::string&) = nullptr);
    ~CaptureWriter();
    bool submit(const Capture& capture);
    bool retry();
    bool busy() const;
    bool failed() const;
    std::optional<SaveResult> poll();

private:
    void work();
    mutable std::mutex mutex_;
    std::condition_variable wake_;
    bool stopping_ = false;
    bool busy_ = false;
    std::shared_ptr<Capture> pending_;
    std::shared_ptr<Capture> failed_;
    std::optional<SaveResult> result_;
    std::string directory_;
    void (*onSaved_)(const std::string&);
    std::thread thread_;
};
}
