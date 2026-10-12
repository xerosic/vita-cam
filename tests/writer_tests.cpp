#include "storage.hpp"
#include <png.h>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <unistd.h>

using namespace cam;
static SaveResult await(CaptureWriter& writer) {
    for (int tries = 0; tries < 1000; ++tries) {
        if (auto result = writer.poll()) return *result;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    assert(false && "Writer timed out");
    return {};
}
int main() {
    char temporary[] = "/tmp/vitacam-writer-XXXXXX";
    const char* dir = mkdtemp(temporary); assert(dir);
    const std::string root(dir);
    Capture capture;
    capture.stem = "immutable";
    capture.frame.pixels.resize(Width*Height,{210,120,30,255});
    {
        CaptureWriter writer(root);
        assert(writer.submit(capture));
        assert(!writer.submit(capture));
        capture.frame.pixels.assign(Width*Height,{0,0,0,255});
        auto saved = await(writer); assert(saved.ok);
        png_image image{}; image.version = PNG_IMAGE_VERSION;
        assert(png_image_begin_read_from_file(&image,saved.path.c_str()));
        image.format = PNG_FORMAT_RGB;
        std::vector<uint8_t> pixels(PNG_IMAGE_SIZE(image));
        assert(png_image_finish_read(&image,nullptr,pixels.data(),0,nullptr));
        assert(pixels[0] == 210 && pixels[1] == 120 && pixels[2] == 30);
        png_image_free(&image);
    }
    {
        CaptureWriter writer(root+"/unavailable");
        capture.stem = "retry";
        assert(writer.submit(capture));
        assert(!await(writer).ok);
        assert(writer.failed() && !writer.busy());
        assert(!writer.submit(capture));
        std::filesystem::create_directory(root+"/unavailable");
        assert(writer.retry());
        assert(await(writer).ok && !writer.failed());
    }
    {
        CaptureWriter writer(root);
        capture.stem = "drain";
        assert(writer.submit(capture));
    }
    assert(std::filesystem::exists(root+"/drain.png"));
    assert(std::filesystem::exists(root+"/drain.json"));
    std::filesystem::remove_all(root);
    std::cout << "Immutable worker capture, queue bounds, failed-save retry and shutdown drain passed\n";
}
