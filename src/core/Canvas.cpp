#include "core/Canvas.h"

#include <algorithm>
#include <fstream>

Color rgb(int r, int g, int b) {
    auto clamp = [](int v) { return static_cast<uint8_t>(std::max(0, std::min(255, v))); };
    return {clamp(r), clamp(g), clamp(b)};
}

Color lerpColor(Color a, Color b, float t) {
    return rgb(static_cast<int>(a.r + (b.r - a.r) * t + 0.5f),
               static_cast<int>(a.g + (b.g - a.g) * t + 0.5f),
               static_cast<int>(a.b + (b.b - a.b) * t + 0.5f));
}

Canvas::Canvas(int w, int h, Color background)
    : w_(w), h_(h), pixels_(static_cast<size_t>(w) * h, background) {}

void Canvas::setPixel(int x, int y, Color c) {
    if (!inBounds(x, y)) return;
    Color& dst = pixels_[static_cast<size_t>(y) * w_ + x];
    switch (op_) {
        case RasterOp::Replace: dst = c; break;
        case RasterOp::Xor: dst = {uint8_t(dst.r ^ c.r), uint8_t(dst.g ^ c.g), uint8_t(dst.b ^ c.b)}; break;
        case RasterOp::And: dst = {uint8_t(dst.r & c.r), uint8_t(dst.g & c.g), uint8_t(dst.b & c.b)}; break;
        case RasterOp::Or:  dst = {uint8_t(dst.r | c.r), uint8_t(dst.g | c.g), uint8_t(dst.b | c.b)}; break;
    }
}

Color Canvas::getPixel(int x, int y) const {
    if (!inBounds(x, y)) return {0, 0, 0};
    return pixels_[static_cast<size_t>(y) * w_ + x];
}

void Canvas::clear(Color c) { std::fill(pixels_.begin(), pixels_.end(), c); }

void Canvas::hline(int x0, int x1, int y, Color c) {
    if (x0 > x1) std::swap(x0, x1);
    for (int x = x0; x <= x1; ++x) setPixel(x, y, c);
}

void Canvas::fillRect(int x, int y, int w, int h, Color c) {
    for (int j = y; j < y + h; ++j) hline(x, x + w - 1, j, c);
}

void Canvas::verticalGradient(Color top, Color bottom) {
    for (int y = 0; y < h_; ++y) hline(0, w_ - 1, y, lerpColor(top, bottom, float(y) / (h_ - 1)));
}

void Canvas::blit(const Canvas& src, int dx, int dy, const Color* transparent) {
    for (int y = 0; y < src.height(); ++y)
        for (int x = 0; x < src.width(); ++x) {
            Color c = src.getPixel(x, y);
            if (transparent && c == *transparent) continue;
            setPixel(dx + x, dy + y, c);
        }
}

void Canvas::blitScaled(const Canvas& src, int dx, int dy, int dw, int dh) {
    for (int y = 0; y < dh; ++y)
        for (int x = 0; x < dw; ++x)
            setPixel(dx + x, dy + y, src.getPixel(x * src.width() / dw, y * src.height() / dh));
}

bool Canvas::savePPM(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << w_ << " " << h_ << "\n255\n";
    for (const Color& c : pixels_) {
        const char px[3] = {char(c.r), char(c.g), char(c.b)};
        out.write(px, 3);
    }
    return static_cast<bool>(out);
}
