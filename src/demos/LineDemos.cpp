// 01 Line drawing and 02 Cohen-Sutherland line clipping.
#include <cmath>
#include <cstdint>

#include "core/Font.h"
#include "demos/Demos.h"
#include "raster/Circles.h"
#include "raster/Lines.h"

namespace demo {

namespace {

// Draws the 12 edges of a box of `size` blocks; the three edges that meet at
// the hidden back-bottom corner are dashed.
void drawWireCube(Canvas& c, const IsoView& v, int x, int y, int z, int size, Color solid, Color hidden) {
    for (int a = 0; a < 8; ++a)
        for (int bit = 0; bit < 3; ++bit) {
            int b = a | (1 << bit);
            if (b == a) continue;
            auto corner = [&](int k) {
                return isoProject(v, x + size * (k & 1), y + size * ((k >> 1) & 1), z + size * ((k >> 2) & 1));
            };
            Vec2 p = corner(a), q = corner(b);
            if (a == 0) drawDashedLine(c, iround(p.x), iround(p.y), iround(q.x), iround(q.y), 6, 4, hidden);
            else drawThickLine(c, iround(p.x), iround(p.y), iround(q.x), iround(q.y), 3, solid);
        }
}

void drawZoomed(Canvas& c, const Canvas& small, int x, int y, int scale) {
    c.blitScaled(small, x, y, small.width() * scale, small.height() * scale);
    Color grid{50, 54, 70};
    for (int i = 0; i <= small.width(); ++i) c.fillRect(x + i * scale, y, 1, small.height() * scale, grid);
    for (int j = 0; j <= small.height(); ++j) c.fillRect(x, y + j * scale, small.width() * scale, 1, grid);
}

std::string outcodeBits(int code) {
    std::string s;
    for (int bit : {TOP, BOTTOM, RIGHT, LEFT}) s += (code & bit) ? '1' : '0';
    return s;
}

}  // namespace

Canvas lines() {
    Canvas c = makePage("01  LINE DRAWING", "DDA + BRESENHAM: WIREFRAME BLOCKS, WORLD GRID, SELECTION");

    // A: star fan in every octant, alternating algorithms.
    panel(c, 10, 76, 385, 256, "DDA VS BRESENHAM (ALL OCTANTS)");
    const Color dda{90, 200, 255}, bres{255, 196, 64};
    for (int i = 0; i < 32; ++i) {
        float a = i * 3.14159265f / 16;
        int x1 = 200 + iround(100 * std::cos(a)), y1 = 212 + iround(100 * std::sin(a));
        if (i % 2 == 0) drawLineDDA(c, 200, 212, x1, y1, dda);
        else drawLineBresenham(c, 200, 212, x1, y1, bres);
    }
    label(c, 24, 304, "DDA", dda);
    label(c, 80, 304, "BRESENHAM", bres);

    // B: the same line rasterised by both algorithms, zoomed 8x.
    panel(c, 405, 76, 385, 256, "PIXELS CHOSEN (ZOOM X8)");
    Canvas a(22, 14, {30, 33, 44}), b(22, 14, {30, 33, 44});
    drawLineDDA(a, 1, 1, 20, 9, dda);
    drawLineBresenham(b, 1, 1, 20, 9, bres);
    int diff = 0;
    for (int y = 0; y < 14; ++y)
        for (int x = 0; x < 22; ++x) diff += (a.getPixel(x, y) == dda) != (b.getPixel(x, y) == bres);
    drawZoomed(c, a, 415, 108, 8);
    drawZoomed(c, b, 605, 108, 8);
    for (int ox : {415, 605})   // the ideal mathematical line through pixel centres
        drawThickLine(c, ox + 12, 108 + 12, ox + 164, 108 + 76, 2, {235, 90, 90});
    label(c, 415, 226, "DDA (FLOAT)", dda);
    label(c, 605, 226, "BRESENHAM (INT)", bres);
    label(c, 415, 252, "RED = IDEAL LINE (1,1)-(20,9)", MUTED);
    label(c, 415, 276, "PIXELS THAT DIFFER: " + std::to_string(diff / 2), TEXT);
    label(c, 415, 300, "BRESENHAM: INTEGER MATHS ONLY", MUTED);

    // C: project use - world grid (DDA) and a block wireframe (thick Bresenham).
    panel(c, 10, 342, 385, 248, "VOXEL WIREFRAME + WORLD GRID");
    IsoView grid{200, 400, 26};
    for (int i = 0; i <= 6; ++i) {
        Vec2 p = isoProject(grid, i, 0, 0), q = isoProject(grid, i, 0, 6);
        drawLineDDA(c, iround(p.x), iround(p.y), iround(q.x), iround(q.y), {80, 86, 110});
        p = isoProject(grid, 0, 0, i), q = isoProject(grid, 6, 0, i);
        drawLineDDA(c, iround(p.x), iround(p.y), iround(q.x), iround(q.y), {80, 86, 110});
    }
    drawWireCube(c, grid, 2, 0, 2, 2, {255, 255, 255}, {140, 146, 170});
    label(c, 20, 566, "DASHED = HIDDEN EDGES", MUTED);

    // D: line styles used by the UI.
    panel(c, 405, 342, 385, 248, "LINE STYLES IN THE UI");
    int ly = 386;
    for (int t : {1, 3, 5}) {
        drawThickLine(c, 420, ly, 560, ly, t, TEXT);
        label(c, 572, ly - 7, std::to_string(t) + " PX");
        ly += 30;
    }
    drawDashedLine(c, 420, ly, 560, ly, 10, 5, ACCENT);
    label(c, 572, ly - 7, "DASH 10/5");
    ly += 30;
    drawDashedLine(c, 420, ly, 560, ly, 3, 3, ACCENT);
    label(c, 572, ly - 7, "DASH 3/3");
    IsoView blk{730, 530, 26};
    drawIsoBlock(c, blk, 0, 0, 0, Block::Grass);
    drawSelection(c, blk, 0, 0, 0, {255, 255, 255});
    label(c, 420, 566, "SELECTED BLOCK OUTLINE ->", MUTED);
    return c;
}

Canvas lineClipping() {
    Canvas c = makePage("02  LINE CLIPPING", "COHEN-SUTHERLAND: CLIP THE WORLD GRID TO THE VIEWPORT");
    const ClipRect area{12, 78, 787, 560};
    const ClipRect win{250, 190, 550, 410};
    const Color region{58, 62, 80};

    // Extended window edges split the plane into the nine outcode regions.
    drawDashedLine(c, iround(win.xmin), 78, iround(win.xmin), 560, 4, 4, region);
    drawDashedLine(c, iround(win.xmax), 78, iround(win.xmax), 560, 4, 4, region);
    drawDashedLine(c, 12, iround(win.ymin), 787, iround(win.ymin), 4, 4, region);
    drawDashedLine(c, 12, iround(win.ymax), 787, iround(win.ymax), 4, 4, region);

    struct Seg { float x0, y0, x1, y1; };
    std::vector<Seg> segs;
    IsoView v{400, 96, 38};   // world floor grid, much bigger than the viewport
    for (int i = 0; i <= 12; ++i) {
        Vec2 p = isoProject(v, i, 0, 0), q = isoProject(v, i, 0, 12);
        segs.push_back({p.x, p.y, q.x, q.y});
        p = isoProject(v, 0, 0, i), q = isoProject(v, 12, 0, i);
        segs.push_back({p.x, p.y, q.x, q.y});
    }
    uint32_t seed = 12345;   // short random "block edges"
    auto rnd = [&](float lo, float hi) {
        seed = seed * 1103515245u + 12345u;
        return lo + (hi - lo) * ((seed >> 8) & 0xFFFF) / 65535.0f;
    };
    for (int i = 0; i < 14; ++i) {
        float x = rnd(40, 760), y = rnd(100, 540), a = rnd(0, 6.283f), len = rnd(30, 110);
        segs.push_back({x, y, x + len * std::cos(a), y + len * std::sin(a)});
    }

    int inside = 0, clipped = 0, rejected = 0;
    for (Seg s : segs) {
        // Keep the drawing inside the page area first.
        if (!clipLineCohenSutherland(s.x0, s.y0, s.x1, s.y1, area)) continue;
        Seg k = s;
        bool accepted = clipLineCohenSutherland(k.x0, k.y0, k.x1, k.y1, win);
        bool changed = k.x0 != s.x0 || k.y0 != s.y0 || k.x1 != s.x1 || k.y1 != s.y1;
        if (!accepted) {
            ++rejected;
            drawDashedLine(c, iround(s.x0), iround(s.y0), iround(s.x1), iround(s.y1), 3, 4, {150, 70, 76});
        } else if (changed) {
            ++clipped;
            drawDashedLine(c, iround(s.x0), iround(s.y0), iround(s.x1), iround(s.y1), 3, 4, {120, 120, 140});
            drawThickLine(c, iround(k.x0), iround(k.y0), iround(k.x1), iround(k.y1), 2, ACCENT);
            fillCircle(c, iround(k.x0), iround(k.y0), 2, TEXT);
            fillCircle(c, iround(k.x1), iround(k.y1), 2, TEXT);
        } else {
            ++inside;
            drawThickLine(c, iround(s.x0), iround(s.y0), iround(s.x1), iround(s.y1), 2, GOOD);
        }
    }
    drawThickLine(c, 250, 190, 550, 190, 2, TEXT); drawThickLine(c, 550, 190, 550, 410, 2, TEXT);
    drawThickLine(c, 550, 410, 250, 410, 2, TEXT); drawThickLine(c, 250, 410, 250, 190, 2, TEXT);
    label(c, 256, 196, "VIEWPORT", TEXT);

    // Outcode (TBRL) label in the middle of each region.
    const float xs[] = {131, 400, 668}, ys[] = {134, 300, 485};
    for (float y : ys)
        for (float x : xs) {
            std::string bits = outcodeBits(computeOutCode(x, y, win));
            int w = textWidth(bits) + 8;
            c.fillRect(iround(x) - w / 2, iround(y) - 11, w, 22, PANEL);
            label(c, iround(x) - w / 2 + 4, iround(y) - 7, bits, TEXT);
        }

    c.fillRect(0, 566, W, 34, PANEL);
    label(c, 16, 576, "INSIDE: " + std::to_string(inside), GOOD);
    label(c, 170, 576, "CLIPPED: " + std::to_string(clipped), ACCENT);
    label(c, 340, 576, "REJECTED: " + std::to_string(rejected), BAD);
    label(c, 530, 576, "OUTCODE = TBRL", MUTED);
    return c;
}

}  // namespace demo
