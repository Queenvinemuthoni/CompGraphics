#pragma once
// Line drawing and line operations.
//   - DDA (Digital Differential Analyzer)
//   - Bresenham (integer decision parameter, all slopes)
//   - Line styles built on Bresenham: thick lines and dashed lines
//   - Cohen-Sutherland line clipping
#include <cstdlib>

#include "core/Canvas.h"
#include "core/Geometry.h"

// Bresenham's algorithm, written as a template so other routines (thick and
// dashed lines) can decide what to do with each pixel the line passes through.
template <typename PlotFn>
void bresenham(int x0, int y0, int x1, int y1, PlotFn plot) {
    int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    if (dx >= dy) {            // |m| <= 1: step along x
        int p = 2 * dy - dx;   // initial decision parameter p0
        for (int x = x0, y = y0;; x += sx) {
            plot(x, y);
            if (x == x1) break;
            if (p >= 0) { y += sy; p += 2 * (dy - dx); }
            else        { p += 2 * dy; }
        }
    } else {                   // |m| > 1: step along y (roles of x and y swapped)
        int p = 2 * dx - dy;
        for (int x = x0, y = y0;; y += sy) {
            plot(x, y);
            if (y == y1) break;
            if (p >= 0) { x += sx; p += 2 * (dx - dy); }
            else        { p += 2 * dx; }
        }
    }
}

void drawLineDDA(Canvas& canvas, int x0, int y0, int x1, int y1, Color c);
void drawLineBresenham(Canvas& canvas, int x0, int y0, int x1, int y1, Color c);
void drawThickLine(Canvas& canvas, int x0, int y0, int x1, int y1, int thickness, Color c);
void drawDashedLine(Canvas& canvas, int x0, int y0, int x1, int y1, int dash, int gap, Color c);
void drawRectOutline(Canvas& canvas, const ClipRect& r, Color c);

// Region outcodes used by Cohen-Sutherland.
enum OutCode : int { INSIDE = 0, LEFT = 1, RIGHT = 2, BOTTOM = 4, TOP = 8 };
int computeOutCode(float x, float y, const ClipRect& r);

// Clips the segment in place. Returns false if the segment lies fully outside.
bool clipLineCohenSutherland(float& x0, float& y0, float& x1, float& y1, const ClipRect& r);
