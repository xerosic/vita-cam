#include "gallery.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <unistd.h>

int main() {
    char temporary[] = "/tmp/vitacam-gallery-XXXXXX";
    const std::string root = mkdtemp(temporary);
    for (const char* name : {"VITA_20261012_142800_1.png", "VITA_20261012_142800_1-Warm-Zoom.png",
                             "VITA_20261012_142900_2.png", "unfinished.png.part", "orphan-Mono.png", "notes.txt"})
        std::ofstream(root + "/" + name) << "test";
    std::filesystem::create_directory(root + "/directory.png");
    std::filesystem::create_symlink(root + "/notes.txt", root + "/link.png");
    const auto photos = cam::scanPhotos(root);
    assert(photos.size() == 2);
    assert(photos[0].image == "VITA_20261012_142900_2.png");
    assert(photos[1].original == "VITA_20261012_142800_1.png");
    assert(photos[1].image == "VITA_20261012_142800_1-Warm-Zoom.png");
    assert(cam::photoLabel(photos[1].original) == "2026-10-12  14:28:00");
    assert(!cam::isPhotoName("../photo.png") && !cam::isPhotoName("a\r\nb.png"));
    assert(cam::scanPhotos(root + "/missing").empty());
    std::filesystem::remove_all(root);
}
