#pragma once
#include "gallery.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace cam {
struct QrCode {
    int size = 0;
    std::vector<uint8_t> modules;
};
QrCode makeQr(const std::string& text);
std::string sharingPage(const std::vector<PhotoEntry>& photos, size_t page);
class PhotoShare {
public:
    PhotoShare();
    ~PhotoShare();
    bool start(const std::string& directory, std::string& error);
    void stop();
    bool active() const;
    bool connected() const;
    const std::string& url() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
