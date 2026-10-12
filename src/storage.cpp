#include "storage.hpp"
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/rtc.h>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace cam {
static std::string readFile(const std::string& path) {
    std::ifstream input(path);
    std::string data(4096, '\0');
    input.read(data.data(), data.size());
    data.resize(size_t(input.gcount()));
    return data;
}
static bool atomicText(const std::string& path, const std::string& content) {
    const auto temporary = path + ".part";
    FILE* f = std::fopen(temporary.c_str(), "wb");
    if (!f)
        return false;
    const bool wrote = std::fwrite(content.data(), 1, content.size(), f) == content.size();
    const int closed = std::fclose(f);
    if (!wrote || closed)
        return false;
    return std::rename(temporary.c_str(), path.c_str()) == 0;
}
bool prepareStorage() {
    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir(DataDirectory, 0777);
    sceIoMkdir(PhotoDirectory, 0777);
    FILE* f = std::fopen("ux0:data/VitaCamera/session.log", "w");
    if (!f)
        return false;
    std::fprintf(f, "Vita Camera %s\n", Version);
    return std::fclose(f) == 0;
}
Settings loadSettings() { return parseSettings(readFile(std::string(DataDirectory) + "/settings.ini")); }
bool storeSettings(const Settings& s) {
    return atomicText(std::string(DataDirectory) + "/settings.ini", settingsText(s));
}
std::string captureName(uint64_t serial) {
    SceDateTime date{};
    sceRtcGetCurrentClockLocalTime(&date);
    char name[96];
    std::snprintf(name, sizeof(name), "VITA_%04u%02u%02u_%02u%02u%02u_%06u_%llu", date.year, date.month, date.day,
                  date.hour, date.minute, date.second, date.microsecond, static_cast<unsigned long long>(serial));
    return name;
}
std::string lastPhoto() {
    const auto path = readFile(std::string(DataDirectory) + "/last-photo.txt");
    const std::string prefix = std::string(PhotoDirectory) + "/";
    if (path.rfind(prefix, 0) != 0 || path.find("..") != std::string::npos || path.find('\n') != std::string::npos)
        return {};
    return path;
}
bool storeLastPhoto(const std::string& path) {
    return atomicText(std::string(DataDirectory) + "/last-photo.txt", path);
}
void commitPhoto(const std::string& path) {
    storeLastPhoto(path);
    sceIoSync("ux0:", 0);
}
}
