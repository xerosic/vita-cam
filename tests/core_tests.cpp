#include "core.hpp"
#include "ui.hpp"
#include <png.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <unistd.h>

using namespace cam;
int main() {
    for (int ev : EvSteps) {
        assert(stepEv(ev, 1) >= ev);
        assert(stepEv(ev, -1) <= ev);
    }
    assert(stepEv(20,1) == 20 && stepEv(-20,-1) == -20);
    assert(cycleValue(2, WhiteBalances.data(), WhiteBalances.size()) == 4);
    assert(cycleValue(0, WhiteBalances.data(), WhiteBalances.size(), -1) == 4);
    Settings settings;
    settings.ev = -7; settings.wb = 4; settings.look = Look::Warm; settings.mirror = false;
    assert(settingsText(parseSettings(settingsText(settings))) == settingsText(settings));
    auto corrupt = parseSettings("ev=999\nwb=3\niso=-1\nlook=999\ngrid=9\nmirror=2\ntimer=2junk\n");
    assert(corrupt.ev == 0 && corrupt.wb == 0 && corrupt.iso == 1);
    assert(corrupt.look == Look::Original && corrupt.grid == Grid::Thirds && corrupt.mirror && corrupt.timer == 0);
    assert(parseSettings("histogram=0\n").graph == Graph::Off);
    assert(parseSettings("graph=2\n").graph == Graph::Waveform);
    assert(parseSettings("graph=90\n").graph == Graph::Luminance);
    Countdown timer;
    timer.start(1000, 2);
    assert(timer.remaining(1000) == 2 && !timer.due(2000999));
    assert(timer.due(2001000) && timer.remaining(2001001) == 0);
    timer.cancel(); assert(!timer.due(999999999));

    Capture capture;
    capture.settings = settings;
    capture.stem = "test-shot";
    capture.frame.id = 42;
    capture.frame.pixels.resize(Width*Height);
    for (int y = 0; y < Height; ++y) for (int x = 0; x < Width; ++x)
        capture.frame.pixels[y*Width+x] = {uint8_t(x%256), uint8_t(y%256), uint8_t((x+y)%256), 0};
    std::vector<Pixel> edited;
    processFrame(capture.frame.pixels, edited, Look::Original, true);
    assert(edited[0].r == capture.frame.pixels[639].r && edited[0].a == 255);
    assert(capture.frame.pixels[0].a == 0);
    processFrame(capture.frame.pixels, edited, Look::Mono);
    assert(edited[521].r == edited[521].g && edited[521].g == edited[521].b);
    assert(processPixel({255,255,255,0},Look::Warm).r == 255);
    std::vector<Pixel> black(640, {0,0,0,255});
    auto a = analyze(black);
    assert(a.shadows == 100 && a.highlights == 0 && a.bins[0] == 160);
    std::vector<Pixel> red(640, {255,0,0,255});
    assert(analyze(red).highlights == 100);
    const auto channels = analyze(red);
    assert(channels.rgb[0][63] == 160 && channels.rgb[1][0] == 160 && channels.rgb[2][0] == 160);
    unsigned density = 0;
    for (auto count : channels.waveform) density += count;
    assert(density == 160);
    assert(analyze({}).peak == 0);
    const auto crop = zoomCrop(2);
    assert(crop.x == 160 && crop.y == 120 && crop.w == 320 && crop.h == 240);
    assert(zoomCrop(100).w == Width/MaxZoom && zoomCrop(-2).w == Width);
    assert(zoomCrop(2,-200,900).x == 0 && zoomCrop(2,-200,900).y == 240);
    std::vector<Pixel> zoomed;
    zoomFrame(capture.frame.pixels,zoomed,1);
    assert(zoomed[0].r == capture.frame.pixels[0].r && zoomed.back().b == capture.frame.pixels.back().b);
    zoomFrame(capture.frame.pixels,zoomed,2);
    assert(zoomed[0].r == 160 && zoomed[0].g == 120);

    char temporary[] = "/tmp/vitacam-tests-XXXXXX";
    const char* dir = mkdtemp(temporary);
    assert(dir);
    const std::string root(dir);
    const auto result = saveCapture(root, capture);
    assert(result.ok && result.originalSaved);
    assert(std::filesystem::exists(root+"/test-shot.png"));
    assert(std::filesystem::exists(root+"/test-shot-Warm.png"));
    assert(std::filesystem::exists(root+"/test-shot.json"));
    png_image image{}; image.version = PNG_IMAGE_VERSION;
    assert(png_image_begin_read_from_file(&image, (root+"/test-shot.png").c_str()));
    assert(image.width == Width && image.height == Height);
    image.format = PNG_FORMAT_RGB;
    std::vector<uint8_t> rgb(PNG_IMAGE_SIZE(image));
    assert(png_image_finish_read(&image, nullptr, rgb.data(), 0, nullptr));
    for (size_t i = 0; i < capture.frame.pixels.size(); ++i) {
        assert(rgb[i*3] == capture.frame.pixels[i].r);
        assert(rgb[i*3+1] == capture.frame.pixels[i].g);
        assert(rgb[i*3+2] == capture.frame.pixels[i].b);
    }
    png_image_free(&image);
    capture.stem = "zoom-shot"; capture.zoom = 2; capture.settings.look = Look::Original;
    const auto zoomResult = saveCapture(root,capture);
    assert(zoomResult.ok && zoomResult.path == root+"/zoom-shot-Zoom.png");
    assert(std::filesystem::exists(root+"/zoom-shot.png"));
    assert(metadata(capture).find("\"digital_zoom\": 2") != std::string::npos);
    image = {}; image.version = PNG_IMAGE_VERSION;
    assert(png_image_begin_read_from_file(&image,zoomResult.path.c_str()));
    image.format = PNG_FORMAT_RGB;
    assert(png_image_finish_read(&image,nullptr,rgb.data(),0,nullptr));
    assert(rgb[0] == 160 && rgb[1] == 120);
    png_image_free(&image);
    capture.stem = "test-shot";
    assert(!saveCapture(root, capture).ok);
    assert(!saveCapture(root+"/missing", capture).originalSaved);
    capture.stem = "../escape"; assert(!saveCapture(root, capture).ok);
    capture.stem = "partial"; capture.frame.pixels.pop_back();
    assert(!saveCapture(root, capture).ok);
    assert(!std::filesystem::exists(root+"/partial.png"));
    std::filesystem::remove_all(root);
    const auto hit = hitTest({{{0,0,50,50},Action::Wb},{{10,10,30,30},Action::Look}},20,20);
    assert(hit.action == Action::Look);
    std::cout << "Capture, PNG round-trip, metadata, settings, timer, analysis and touch checks passed\n";
}
