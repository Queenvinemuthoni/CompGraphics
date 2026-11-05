#include "raster/Circles.h"

void drawCircle(Canvas& canvas, int xc, int yc, int r, Color c) {
    midpointCircle(r, [&](int x, int y) {
        // One computed point gives eight pixels by symmetry.
        canvas.setPixel(xc + x, yc + y, c); canvas.setPixel(xc - x, yc + y, c);
        canvas.setPixel(xc + x, yc - y, c); canvas.setPixel(xc - x, yc - y, c);
        canvas.setPixel(xc + y, yc + x, c); canvas.setPixel(xc - y, yc + x, c);
        canvas.setPixel(xc + y, yc - x, c); canvas.setPixel(xc - y, yc - x, c);
    });
}

void fillCircle(Canvas& canvas, int xc, int yc, int r, Color c) {
    midpointCircle(r, [&](int x, int y) {
        canvas.hline(xc - x, xc + x, yc + y, c);
        canvas.hline(xc - x, xc + x, yc - y, c);
        canvas.hline(xc - y, xc + y, yc + x, c);
        canvas.hline(xc - y, xc + y, yc - x, c);
    });
}

void drawEllipse(Canvas& canvas, int xc, int yc, int rx, int ry, Color c) {
    midpointEllipse(rx, ry, [&](int x, int y) {
        canvas.setPixel(xc + x, yc + y, c); canvas.setPixel(xc - x, yc + y, c);
        canvas.setPixel(xc + x, yc - y, c); canvas.setPixel(xc - x, yc - y, c);
    });
}

void fillEllipse(Canvas& canvas, int xc, int yc, int rx, int ry, Color c) {
    midpointEllipse(rx, ry, [&](int x, int y) {
        canvas.hline(xc - x, xc + x, yc + y, c);
        canvas.hline(xc - x, xc + x, yc - y, c);
    });
}
