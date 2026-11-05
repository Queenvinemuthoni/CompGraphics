#pragma once
// Canvas: a software framebuffer. Every algorithm in this project writes
// pixels through Canvas::setPixel - no graphics library is used.
#include <cstdint>
#include <string>
#include <vector>

struct Color {
    uint8_t r = 0, g = 0, b = 0;
    bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b; }
    bool operator!=(const Color& o) const { return !(*this == o); }
};

// Builds a colour from ints, clamping each channel to 0..255.
Color rgb(int r, int g, int b);
// Linear interpolation between two colours, t in [0,1].
Color lerpColor(Color a, Color b, float t);

// Raster operations: how a new pixel is combined with the pixel already there.
enum class RasterOp { Replace, Xor, And, Or };

class Canvas {
public:
    Canvas(int w, int h, Color background = {0, 0, 0});

    int width() const { return w_; }
    int height() const { return h_; }
    bool inBounds(int x, int y) const { return x >= 0 && x < w_ && y >= 0 && y < h_; }

    void setPixel(int x, int y, Color c);   // honours the current RasterOp
    Color getPixel(int x, int y) const;

    void setRasterOp(RasterOp op) { op_ = op; }
    RasterOp rasterOp() const { return op_; }

    void clear(Color c);
    void hline(int x0, int x1, int y, Color c);
    void fillRect(int x, int y, int w, int h, Color c);
    void verticalGradient(Color top, Color bottom);

    // Block transfer (BitBlt). Pixels equal to *transparent are skipped (colour-key mask).
    void blit(const Canvas& src, int dx, int dy, const Color* transparent = nullptr);
    // Nearest-neighbour scaled copy, used to build side-by-side comparison panels.
    void blitScaled(const Canvas& src, int dx, int dy, int dw, int dh);

    // Binary PPM (P6). Returns false if the file could not be written.
    bool savePPM(const std::string& path) const;

private:
    int w_, h_;
    std::vector<Color> pixels_;
    RasterOp op_ = RasterOp::Replace;
};
