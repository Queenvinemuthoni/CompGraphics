// 09 Raster operations, 10 visual concept and 11 the 3D preview.
#include "core/Font.h"
#include "demos/Demos.h"
#include "raster/Lines.h"
#include "raster/Polygons.h"
#include "scene/Preview3D.h"

namespace demo {

namespace {

bool sameImage(const Canvas& a, const Canvas& b) {
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width(); ++x)
            if (a.getPixel(x, y) != b.getPixel(x, y)) return false;
    return true;
}

void xorRect(Canvas& c, int x0, int y0, int x1, int y1) {
    RasterOp old = c.rasterOp();
    c.setRasterOp(RasterOp::Xor);
    for (int t = 0; t < 2; ++t) {   // 2-pixel frame, each pixel touched exactly once
        c.hline(x0 + t, x1 - t, y0 + t, {255, 255, 255});
        c.hline(x0 + t, x1 - t, y1 - t, {255, 255, 255});
        for (int y = y0 + 2; y <= y1 - 2; ++y) {
            c.setPixel(x0 + t, y, {255, 255, 255});
            c.setPixel(x1 - t, y, {255, 255, 255});
        }
    }
    c.setRasterOp(old);
}

}  // namespace

Canvas rasterOps() {
    Canvas c = makePage("09  RASTER OPERATIONS", "XOR CURSOR, BITBLT WITH COLOUR KEY, AND/OR SPRITE MASKING");
    Canvas full = renderConcept();

    // A: XOR rubber-band selection - drawing it twice restores the image.
    panel(c, 10, 76, 385, 300, "XOR SELECTION BOX");
    Canvas view(365, 236);
    view.blit(full, -200, -150);
    Canvas original = view;
    xorRect(view, 60, 50, 250, 170);
    Canvas twice = view;
    xorRect(twice, 60, 50, 250, 170);
    c.blit(view, 20, 104);
    bool restored = sameImage(twice, original);
    label(c, 20, 348, std::string("XOR TWICE = ORIGINAL: ") + (restored ? "YES" : "NO"), restored ? GOOD : BAD);

    // B: BitBlt of hotbar icons, raw vs with the magenta colour key skipped.
    panel(c, 405, 76, 385, 300, "BITBLT + COLOUR KEY");
    const Color key{255, 0, 255};
    const Block icons[] = {Block::Grass, Block::Stone, Block::Water, Block::Wood};
    Canvas bg(365, 104);
    bg.blit(full, -40, -330);
    c.blit(bg, 415, 254);
    for (int i = 0; i < 4; ++i) {
        Canvas icon = makeBlockIcon(icons[i], 64, key);
        c.blit(icon, 425 + i * 88, 132);            // raw copy: key colour visible
        c.blit(icon, 425 + i * 88, 274, &key);      // keyed copy: transparent
    }
    label(c, 415, 108, "SPRITE SHEET (RAW)", MUTED);
    label(c, 415, 210, "BLITTED, MAGENTA SKIPPED:", MUTED);

    // C: classic sprite masking: (background AND mask) OR sprite.
    panel(c, 10, 386, 780, 204, "SPRITE MASKING: (BACKGROUND AND MASK) OR SPRITE");
    const int S = 136;
    IsoView v{S / 2.0f, S / 2.0f, 58};
    Canvas back(S, S);
    back.blit(full, -470, -40);
    Canvas mask(S, S, {255, 255, 255});
    Polygon sil = isoBlockSilhouette(v, 0, 0, 0);
    fillPolygonScanline(mask, sil, {0, 0, 0});
    drawPolygonOutline(mask, sil, {0, 0, 0});
    Canvas sprite(S, S, {0, 0, 0});
    drawIsoBlock(sprite, v, 0, 0, 0, Block::Grass);
    Canvas anded = back;
    anded.setRasterOp(RasterOp::And);
    anded.blit(mask, 0, 0);
    Canvas result = anded;
    result.setRasterOp(RasterOp::Or);
    result.blit(sprite, 0, 0);

    const Canvas* steps[] = {&back, &mask, &anded, &sprite, &result};
    const char* names[] = {"BACKGROUND", "MASK", "BG AND MASK", "SPRITE", "... OR SPRITE"};
    for (int i = 0; i < 5; ++i) {
        int x = 22 + i * 154;
        c.blit(*steps[i], x, 414);
        drawRectOutline(c, {float(x - 1), 413, float(x + S), float(414 + S)}, {90, 96, 120});
        label(c, x, 560, names[i], i == 4 ? ACCENT : TEXT);
    }
    return c;
}

Canvas visualConcept() { return renderConcept(); }

Canvas preview3D() {
    Canvas c = makePage("11  3D PREVIEW (TOWARDS CP2/CP3)", "PERSPECTIVE, Z-BUFFER, FACE CULLING, DIFFUSE LIGHT");
    Canvas view(780, 516, {30, 33, 44});
    renderPreview3D(view);
    c.blit(view, 10, 76);
    drawRectOutline(c, {10, 76, 789, 591}, {58, 62, 80});
    return c;
}

}  // namespace demo
