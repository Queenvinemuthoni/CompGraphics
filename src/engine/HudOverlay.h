#pragma once
// The 2D HUD is drawn on the CPU with our own raster algorithms (CP1) into a
// Canvas, uploaded as a texture, and blended over the 3D view. Magenta pixels
// in the canvas become fully transparent.
#include "core/Canvas.h"
#include "engine/GlUtil.h"

class HudOverlay {
public:
    HudOverlay(int width, int height);
    ~HudOverlay();

    Canvas& canvas() { return canvas_; }
    void clear() { canvas_.clear(KEY); }
    void upload();   // call after drawing into canvas()
    void draw() const;

    static constexpr Color KEY{255, 0, 255};

private:
    Canvas canvas_;
    std::vector<unsigned char> rgba_;
    GLuint texture_ = 0, vao_ = 0, program_ = 0;
};
