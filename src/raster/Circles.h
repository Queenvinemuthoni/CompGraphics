#pragma once
// Circle and ellipse representation.
//   - Midpoint circle algorithm (8-way symmetry)
//   - Midpoint ellipse algorithm (two regions, 4-way symmetry)
//   - Filled versions that turn the symmetric points into horizontal spans
#include "core/Canvas.h"

// Calls plot(dx, dy) for every point of one octant of a circle of radius r
// centred on the origin; the caller expands it using symmetry.
template <typename OctantFn>
void midpointCircle(int r, OctantFn plot) {
    int x = 0, y = r;
    int p = 1 - r;                 // initial decision parameter
    while (x <= y) {
        plot(x, y);
        ++x;
        if (p < 0) p += 2 * x + 1;
        else { --y; p += 2 * (x - y) + 1; }
    }
}

// Calls plot(dx, dy) for every point of one quadrant of an ellipse.
template <typename QuadrantFn>
void midpointEllipse(int rx, int ry, QuadrantFn plot) {
    const double rx2 = double(rx) * rx, ry2 = double(ry) * ry;
    long x = 0, y = ry;
    double px = 0, py = 2 * rx2 * y;
    // Region 1: slope magnitude < 1, step in x.
    double p1 = ry2 - rx2 * ry + 0.25 * rx2;
    while (px < py) {
        plot(int(x), int(y));
        ++x;
        px += 2 * ry2;
        if (p1 < 0) p1 += ry2 + px;
        else { --y; py -= 2 * rx2; p1 += ry2 + px - py; }
    }
    // Region 2: slope magnitude >= 1, step in y.
    double p2 = ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1.0) * (y - 1.0) - rx2 * ry2;
    while (y >= 0) {
        plot(int(x), int(y));
        --y;
        py -= 2 * rx2;
        if (p2 > 0) p2 += rx2 - py;
        else { ++x; px += 2 * ry2; p2 += rx2 - py + px; }
    }
}

void drawCircle(Canvas& canvas, int xc, int yc, int r, Color c);
void fillCircle(Canvas& canvas, int xc, int yc, int r, Color c);
void drawEllipse(Canvas& canvas, int xc, int yc, int rx, int ry, Color c);
void fillEllipse(Canvas& canvas, int xc, int yc, int rx, int ry, Color c);
