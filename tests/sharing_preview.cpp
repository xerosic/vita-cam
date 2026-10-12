#include "core.hpp"
#include "sharing.hpp"
#include <filesystem>
#include <iostream>

int main() {
    const std::string root = "build-host/share-fixtures";
    std::filesystem::create_directories(root);
    std::vector<cam::Pixel> pixels(cam::Width*cam::Height);
    std::string error;
    for (int photo = 0; photo < 13; ++photo) {
        for (int y = 0; y < cam::Height; ++y) for (int x = 0; x < cam::Width; ++x)
            pixels[y*cam::Width+x] = {uint8_t(20+x*100/cam::Width+photo*5),uint8_t(65+y*130/cam::Height),uint8_t(125+x*75/cam::Width),255};
        if (!cam::writePng(root+"/sample_"+std::to_string(photo)+".png",pixels,error)) return 1;
    }
    cam::PhotoShare share;
    if (!share.start(root,error)) { std::cerr << error << '\n'; return 1; }
    std::cout << share.url() << std::endl;
    std::cin.get();
}
