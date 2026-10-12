#include "sharing.hpp"
#include "qrcodegen.h"
#include <algorithm>
#include <sstream>

namespace cam {
QrCode makeQr(const std::string& text) {
    std::vector<uint8_t> scratch(qrcodegen_BUFFER_LEN_MAX), data(qrcodegen_BUFFER_LEN_MAX);
    QrCode qr;
    if (!qrcodegen_encodeText(text.c_str(),scratch.data(),data.data(),qrcodegen_Ecc_MEDIUM,
            qrcodegen_VERSION_MIN,qrcodegen_VERSION_MAX,qrcodegen_Mask_AUTO,true)) return qr;
    qr.size = qrcodegen_getSize(data.data());
    qr.modules.resize(qr.size*qr.size);
    for (int y = 0; y < qr.size; ++y) for (int x = 0; x < qr.size; ++x)
        qr.modules[y*qr.size+x] = qrcodegen_getModule(data.data(),x,y);
    return qr;
}
std::string sharingPage(const std::vector<PhotoEntry>& photos, size_t page) {
    constexpr size_t perPage = 12;
    const size_t pages = std::max(size_t(1),(photos.size()+perPage-1)/perPage);
    page = std::min(page,pages-1);
    std::ostringstream out;
    out << R"HTML(<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover"><meta name="color-scheme" content="dark"><title>Vita Camera · Photos</title><style>
*{box-sizing:border-box}body{margin:0;background:#071525;color:#eff8ff;font:16px -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;line-height:1.5}main{max-width:1080px;margin:auto;padding:28px max(18px,env(safe-area-inset-right)) max(28px,env(safe-area-inset-bottom)) max(18px,env(safe-area-inset-left))}header{margin-bottom:26px}h1{font-size:26px;letter-spacing:-.6px;margin:0 0 8px}p{margin:0;color:#abc6db}small{color:#8faec8}.photos{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:20px}article{min-width:0;background:#102943;border:1px solid #224562;border-radius:16px;overflow:hidden}article>a{display:block}img{display:block;width:100%;height:auto;aspect-ratio:4/3;object-fit:contain;background:#020c16}section{padding:14px}h2{font-size:14px;font-weight:500;margin:0 0 12px;overflow-wrap:anywhere}a{color:#8edaff;text-underline-offset:4px}a:focus-visible{outline:3px solid #8edaff;outline-offset:3px}.links{display:flex;gap:10px;flex-wrap:wrap}.links a,nav a{display:inline-flex;align-items:center;justify-content:center;min-height:44px;padding:9px 14px;border:1px solid #39627f;border-radius:24px;text-decoration:none}.links a:first-child{background:#17496d}nav{display:flex;justify-content:space-between;align-items:center;gap:12px;margin-top:24px}footer{margin-top:28px;color:#8faec8;font-size:13px}@media(max-width:560px){.photos{grid-template-columns:1fr}main{padding-top:20px}}@media(prefers-reduced-motion:no-preference){a{transition:background-color .15s}}
</style><main><header><h1>Vita Camera</h1><p>Touch and hold a photo to save it to Photos.</p><small>)HTML";
    out << photos.size() << " photos</small></header><div class=\"photos\">";
    for (size_t i = page*perPage; i < std::min(photos.size(),(page+1)*perPage); ++i) {
        const auto& photo = photos[i];
        if (!isPhotoName(photo.original) || !isPhotoName(photo.image)) continue;
        const std::string path = "photo/"+photo.image;
        out << "<article><a href=\"" << path << "\"><img loading=\"lazy\" width=\"640\" height=\"480\" src=\"" << path
            << "\" alt=\"Photo " << i+1 << "\"></a><section><h2>" << photoLabel(photo.original) << "</h2><div class=\"links\">"
            << "<a href=\"" << path << "?download=1\" download=\"" << photo.image << "\">Download PNG</a>";
        if (photo.original != photo.image) out << "<a href=\"photo/" << photo.original << "\">Full original</a>";
        out << "</div></section></article>";
    }
    out << "</div>";
    if (photos.empty()) out << "<p>No photos yet. Take a photo on your Vita, then reopen Share.</p>";
    if (pages > 1) {
        out << "<nav aria-label=\"Photo pages\">";
        if (page) out << "<a href=\"?page=" << page-1 << "\">Previous</a>";
        out << "<span>" << page+1 << " / " << pages << "</span>";
        if (page+1 < pages) out << "<a href=\"?page=" << page+1 << "\">Next</a>";
        out << "</nav>";
    }
    out << "<footer>Keep Share open on your Vita while saving photos.</footer></main></html>";
    return out.str();
}
}
