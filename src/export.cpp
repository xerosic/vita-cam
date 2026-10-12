#include "core.hpp"
#include <png.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace cam {
bool writePng(const std::string& path, const std::vector<Pixel>& pixels, std::string& error) {
    if (pixels.size() != size_t(Width * Height)) {
        error = "Incomplete camera frame";
        return false;
    }
    FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) {
        error = std::strerror(errno);
        return false;
    }
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info = png ? png_create_info_struct(png) : nullptr;
    if (!png || !info) {
        if (png)
            png_destroy_write_struct(&png, nullptr);
        std::fclose(file);
        error = "PNG allocation failed";
        return false;
    }
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        std::fclose(file);
        error = "PNG write failed";
        return false;
    }
    png_init_io(png, file);
    png_set_IHDR(png, info, Width, Height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_set_compression_level(png, 1);
    png_write_info(png, info);
    png_set_filler(png, 0, PNG_FILLER_AFTER);
    for (int y = 0; y < Height; ++y)
        png_write_row(png, reinterpret_cast<png_const_bytep>(pixels.data() + y * Width));
    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);
    if (std::fclose(file)) {
        error = std::strerror(errno);
        return false;
    }
    return true;
}
static bool exists(const std::string& path) {
    struct stat st{};
    return stat(path.c_str(), &st) == 0;
}
static bool promote(const std::string& part, const std::string& final, std::string& error) {
    if (exists(final)) {
        error = "Capture filename already exists";
        return false;
    }
    if (std::rename(part.c_str(), final.c_str())) {
        error = std::strerror(errno);
        return false;
    }
    return true;
}
SaveResult saveCapture(const std::string& directory, const Capture& c) {
    SaveResult r;
    if (c.stem.empty() ||
        c.stem.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_") !=
            std::string::npos) {
        r.error = "Invalid capture name";
        return r;
    }
    const std::string base = directory + "/" + c.stem;
    const std::string original = base + ".png";
    const bool zoomed = zoomCrop(c.zoom).w < Width;
    const bool hasEdit = c.settings.look != Look::Original || zoomed;
    const std::string edited =
        base + (c.settings.look != Look::Original ? std::string("-") + LookNames[int(c.settings.look)] : "") +
        (zoomed ? "-Zoom" : "") + ".png";
    if (exists(original) || exists(base + ".json") || (hasEdit && exists(edited))) {
        r.error = "Capture filename already exists";
        return r;
    }
    if (!writePng(original + ".part", c.frame.pixels, r.error)) {
        std::remove((original + ".part").c_str());
        return r;
    }
    if (!promote(original + ".part", original, r.error))
        return r;
    r.originalSaved = true;
    r.path = original;
    if (hasEdit) {
        std::vector<Pixel> pixels;
        processFrame(c.frame.pixels, pixels, c.settings.look);
        if (zoomed) {
            std::vector<Pixel> cropped;
            zoomFrame(pixels, cropped, c.zoom);
            pixels.swap(cropped);
        }
        if (!writePng(edited + ".part", pixels, r.error)) {
            std::remove((edited + ".part").c_str());
            return r;
        }
        if (!promote(edited + ".part", edited, r.error))
            return r;
        r.path = edited;
    }
    const auto json = metadata(c);
    const auto path = base + ".json";
    FILE* file = std::fopen((path + ".part").c_str(), "wb");
    if (!file) {
        r.error = std::strerror(errno);
        return r;
    }
    const bool written = std::fwrite(json.data(), 1, json.size(), file) == json.size();
    const int closed = std::fclose(file);
    if (!written || closed) {
        r.error = "Settings write failed";
        return r;
    }
    if (!promote(path + ".part", path, r.error))
        return r;
    r.ok = true;
    return r;
}
}
