#include "gallery.hpp"
#include "core.hpp"
#include <algorithm>
#include <dirent.h>
#include <set>
#include <sys/stat.h>

namespace cam {
bool isPhotoName(const std::string& name) {
    return name.size() > 4 && name.size() < 240 && name.compare(name.size()-4,4,".png") == 0
        && name.substr(0,name.size()-4).find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_") == std::string::npos;
}
static std::string originalName(std::string name) {
    name.resize(name.size()-4);
    const auto trim = [&](const std::string& suffix) {
        if (name.size() > suffix.size() && name.compare(name.size()-suffix.size(),suffix.size(),suffix) == 0) {
            name.resize(name.size()-suffix.size()); return true;
        }
        return false;
    };
    trim("-Zoom");
    for (size_t i = 1; i < LookNames.size(); ++i) if (trim(std::string("-")+LookNames[i])) break;
    return name+".png";
}
std::vector<PhotoEntry> scanPhotos(const std::string& directory) {
    std::vector<PhotoEntry> photos;
    DIR* dir = opendir(directory.c_str());
    if (!dir) return photos;
    std::set<std::string> names;
    while (auto* entry = readdir(dir)) {
        const std::string name = entry->d_name;
        if (!isPhotoName(name)) continue;
        struct stat info{};
        const std::string path = directory+"/"+name;
#ifdef __vita__
        const int result = stat(path.c_str(),&info);
#else
        const int result = lstat(path.c_str(),&info);
#endif
        if (result == 0 && S_ISREG(info.st_mode)) names.insert(name);
    }
    closedir(dir);
    for (auto it = names.rbegin(); it != names.rend(); ++it) {
        if (originalName(*it) != *it) continue;
        PhotoEntry photo{*it,*it};
        const std::string stem = it->substr(0,it->size()-4);
        for (size_t look = 0; look < LookNames.size(); ++look) {
            const std::string base = stem+(look ? std::string("-")+LookNames[look] : "");
            for (const auto* suffix : {".png","-Zoom.png"}) if (names.count(base+suffix)) photo.image = base+suffix;
        }
        photos.push_back(std::move(photo));
    }
    return photos;
}
std::string photoLabel(const std::string& name) {
    if (name.size() >= 19 && name.rfind("VITA_",0) == 0 && name[13] == '_')
        return name.substr(5,4)+"-"+name.substr(9,2)+"-"+name.substr(11,2)+"  "+name.substr(14,2)+":"+name.substr(16,2)+":"+name.substr(18,2);
    return name.substr(0,name.size()-4);
}
}
