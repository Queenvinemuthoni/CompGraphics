#include "raster/Lines.h"

#include <algorithm>
#include <cmath>

void drawLineDDA(Canvas& canvas, int x0, int y0, int x1, int y1, Color c) {
    int dx = x1 - x0, dy = y1 - y0;
    int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) { canvas.setPixel(x0, y0, c); return; }
    float xInc = dx / static_cast<float>(steps);
    float yInc = dy / static_cast<float>(steps);
    float x = static_cast<float>(x0), y = static_cast<float>(y0);
    for (int i = 0; i <= steps; ++i) {
        canvas.setPixel(iround(x), iround(y), c);
        x += xInc;
        y += yInc;
    }
}

void drawLineBresenham(Canvas& canvas, int x0, int y0, int x1, int y1, Color c) {
    bresenham(x0, y0, x1, y1, [&](int x, int y) { canvas.setPixel(x, y, c); });
}

void drawThickLine(Canvas& canvas, int x0, int y0, int x1, int y1, int thickness, Color c) {
    // Stamp a square brush at every Bresenham pixel.
    int half = thickness / 2;
    bresenham(x0, y0, x1, y1, [&](int x, int y) {
        canvas.fillRect(x - half, y - half, thickness, thickness, c);
    });
}

void drawDashedLine(Canvas& canvas, int x0, int y0, int x1, int y1, int dash, int gap, Color c) {
    // A pixel counter decides whether we are inside a dash or a gap.
    int counter = 0;
    bresenham(x0, y0, x1, y1, [&](int x, int y) {
        if (counter % (dash + gap) < dash) canvas.setPixel(x, y, c);
        ++counter;
    });
}

void drawRectOutline(Canvas& canvas, const ClipRect& r, Color c) {
    int x0 = iround(r.xmin), y0 = iround(r.ymin), x1 = iround(r.xmax), y1 = iround(r.ymax);
    drawLineBresenham(canvas, x0, y0, x1, y0, c);
    drawLineBresenham(canvas, x1, y0, x1, y1, c);
    drawLineBresenham(canvas, x1, y1, x0, y1, c);
    drawLineBresenham(canvas, x0, y1, x0, y0, c);
}

int computeOutCode(float x, float y, const ClipRect& r) {
    int code = INSIDE;
    if (x < r.xmin) code |= LEFT;
    else if (x > r.xmax) code |= RIGHT;
    if (y < r.ymin) code |= TOP;          // screen y grows downwards
    else if (y > r.ymax) code |= BOTTOM;
    return code;
}

bool clipLineCohenSutherland(float& x0, float& y0, float& x1, float& y1, const ClipRect& r) {
    int code0 = computeOutCode(x0, y0, r);
    int code1 = computeOutCode(x1, y1, r);
    while (true) {
        if (!(code0 | code1)) return true;   // trivially accept: both inside
        if (code0 & code1) return false;     // trivially reject: share an outside region
        // Pick an endpoint that is outside and move it to the window boundary.
        int out = code0 ? code0 : code1;
        float x = 0, y = 0;
        if (out & TOP)         { x = x0 + (x1 - x0) * (r.ymin - y0) / (y1 - y0); y = r.ymin; }
        else if (out & BOTTOM) { x = x0 + (x1 - x0) * (r.ymax - y0) / (y1 - y0); y = r.ymax; }
        else if (out & RIGHT)  { y = y0 + (y1 - y0) * (r.xmax - x0) / (x1 - x0); x = r.xmax; }
        else                   { y = y0 + (y1 - y0) * (r.xmin - x0) / (x1 - x0); x = r.xmin; }
        if (out == code0) { x0 = x; y0 = y; code0 = computeOutCode(x0, y0, r); }
        else              { x1 = x; y1 = y; code1 = computeOutCode(x1, y1, r); }
    }
}
