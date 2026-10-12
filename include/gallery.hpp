#pragma once
#include <string>
#include <vector>

namespace cam {
struct PhotoEntry {
    std::string original, image;
};
bool isPhotoName(const std::string& name);
std::vector<PhotoEntry> scanPhotos(const std::string& directory);
std::string photoLabel(const std::string& name);
}
