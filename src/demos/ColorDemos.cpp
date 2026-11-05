// 07 Colour theory and 08 Colour space reduction.
#include <cmath>
#include <cstdio>

#include "color/ColorOps.h"
#include "core/Font.h"
#include "demos/Demos.h"
#include "raster/Circles.h"
#include "raster/Lines.h"

namespace demo {

Canvas colorTheory() {
    Canvas c = makePage("07  COLOUR THEORY", "HSV WHEEL, ADDITIVE RGB, SUBTRACTIVE CMY, BLOCK SHADING");
    const Block blocks[] = {Block::Grass, Block::Dirt, Block::Stone, Block::Sand,
                            Block::Water, Block::Wood, Block::Leaves};

    // A: HSV wheel (hue = angle, saturation = radius) with every block colour marked.
    panel(c, 10, 76, 253, 282, "HSV COLOUR WHEEL");
    const int cx = 136, cy = 226, r = 100;
    for (int y = -r; y <= r; ++y)
        for (int x = -r; x <= r; ++x) {
            float d = std::sqrt(float(x * x + y * y));
            if (d > r) continue;
            float h = std::atan2(float(y), float(x)) * 180.0f / 3.14159265f;
            c.setPixel(cx + x, cy + y, hsvToRgb({h < 0 ? h + 360 : h, d / r, 1.0f}));
        }
    drawCircle(c, cx, cy, r, TEXT);
    for (Block b : blocks) {
        HSV hsv = rgbToHsv(blockBaseColor(b));
        float a = hsv.h * 3.14159265f / 180.0f;
        int px = cx + iround(std::cos(a) * hsv.s * r), py = cy + iround(std::sin(a) * hsv.s * r);
        fillCircle(c, px, py, 5, blockBaseColor(b));
        drawCircle(c, px, py, 6, {20, 20, 20});
    }
    label(c, 20, 334, "DOTS = BLOCK COLOURS", MUTED);

    // B: additive mixing with the OR raster op on black.
    panel(c, 273, 76, 254, 282, "ADDITIVE RGB (OR)");
    c.fillRect(283, 104, 234, 222, {0, 0, 0});
    c.setRasterOp(RasterOp::Or);
    fillCircle(c, 368, 190, 58, {255, 0, 0});
    fillCircle(c, 432, 190, 58, {0, 255, 0});
    fillCircle(c, 400, 246, 58, {0, 0, 255});
    c.setRasterOp(RasterOp::Replace);
    label(c, 283, 334, "LIGHT: SCREENS", MUTED);

    // C: subtractive mixing with the AND raster op on white.
    panel(c, 537, 76, 253, 282, "SUBTRACTIVE CMY (AND)");
    c.fillRect(547, 104, 233, 222, {255, 255, 255});
    c.setRasterOp(RasterOp::And);
    fillCircle(c, 631, 190, 58, {0, 255, 255});
    fillCircle(c, 695, 190, 58, {255, 0, 255});
    fillCircle(c, 663, 246, 58, {255, 255, 0});
    c.setRasterOp(RasterOp::Replace);
    label(c, 547, 334, "INK: PRINTING", MUTED);

    // D: each block's three face shades come from scaling V in HSV space.
    panel(c, 10, 368, 780, 222, "BLOCK PALETTE: FACE SHADES = HSV VALUE X 1.0 / 0.8 / 0.62");
    for (int i = 0; i < 7; ++i) {
        int colX = 20 + i * 110;
        Block b = blocks[i];
        drawIsoBlock(c, IsoView{float(colX + 50), 420, 28}, 0, 0, 0, b);
        Color base = blockBaseColor(b);
        const float f[] = {1.0f, 0.8f, 0.62f};
        for (int k = 0; k < 3; ++k) {
            c.fillRect(colX + 8 + k * 30, 492, 26, 20, shade(base, f[k]));
            drawRectOutline(c, {float(colX + 8 + k * 30), 492, float(colX + 33 + k * 30), 511}, {20, 20, 20});
        }
        label(c, colX + 8, 522, blockName(b), TEXT);
        HSV hsv = rgbToHsv(base);
        char buf[48];
        std::snprintf(buf, sizeof buf, "H%03.0f S%02.0f V%02.0f", hsv.h, hsv.s * 99, hsv.v * 99);
        label(c, colX + 8, 546, buf, MUTED, 1);
        std::snprintf(buf, sizeof buf, "RGB %d,%d,%d", base.r, base.g, base.b);
        label(c, colX + 8, 560, buf, MUTED, 1);
    }
    return c;
}

Canvas colorReduction() {
    Canvas c = makePage("08  COLOUR SPACE REDUCTION", "QUANTISATION AND DITHERING OF THE GAME SCENE");
    Canvas full = renderConcept();
    const int cw = 248, ch = 186;
    Canvas source(cw, ch);
    source.blit(full, -250, -170);   // crop around the island

    const std::vector<Color> pal27 = uniformPalette(3, 3, 3);
    struct Variant { const char* title; int mode; };
    const Variant variants[] = {{"ORIGINAL", 0}, {"GREYSCALE", 1}, {"27 COLOURS: NEAREST", 2},
                                {"27 COLOURS: ORDERED", 3}, {"27: FLOYD-STEINBERG", 4}, {"16: POPULARITY", 5}};
    for (int i = 0; i < 6; ++i) {
        int px = 10 + (i % 3) * 261, py = 76 + (i / 3) * 258;
        panel(c, px, py, 258, 252, variants[i].title);
        Canvas img = source;
        switch (variants[i].mode) {
            case 1: toGrayscale(img); break;
            case 2: quantize(img, pal27); break;
            case 3: ditherOrdered(img, pal27, 127.5f); break;
            case 4: ditherFloydSteinberg(img, pal27); break;
            case 5: quantize(img, popularityPalette(img, 16)); break;
            default: break;
        }
        c.blit(img, px + 5, py + 30);
        label(c, px + 8, py + 224, std::to_string(countDistinctColors(img)) + " COLOURS", TEXT);
    }
    return c;
}

}  // namespace demo
