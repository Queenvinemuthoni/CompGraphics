// Shared page layout and the Voxel World Builder scene pieces used by the demos.
#include <cmath>

#include "color/ColorOps.h"
#include "core/Font.h"
#include "demos/Demos.h"
#include "raster/Circles.h"
#include "raster/Curves.h"
#include "raster/Lines.h"
#include "raster/Polygons.h"

namespace demo {

const Color BG{24, 26, 34}, PANEL{36, 39, 52}, TEXT{236, 236, 242}, MUTED{150, 156, 172},
    ACCENT{255, 196, 64}, GOOD{110, 220, 120}, BAD{235, 90, 90};

Canvas makePage(const std::string& title, const std::string& subtitle) {
    Canvas c(W, H, BG);
    c.fillRect(0, 0, W, 64, PANEL);
    c.fillRect(0, 64, W, 2, ACCENT);
    drawText(c, 16, 10, title, TEXT, 3);
    drawText(c, 16, 40, subtitle, MUTED, 2);
    return c;
}

void label(Canvas& c, int x, int y, const std::string& text, Color col, int scale) {
    drawText(c, x, y, text, col, scale);
}

void panel(Canvas& c, int x, int y, int w, int h, const std::string& title) {
    c.fillRect(x, y, w, h, {30, 33, 44});
    drawRectOutline(c, {float(x), float(y), float(x + w - 1), float(y + h - 1)}, {58, 62, 80});
    if (!title.empty()) drawText(c, x + 8, y + 8, title, ACCENT, 2);
}

VoxelWorld makeWorld() {
    VoxelWorld world(15, 13, 15);
    world.generateTerrain();
    return world;
}

void drawSkyColors(Canvas& c, int top, int bottom, Color skyTop, Color skyBottom) {
    for (int y = top; y < bottom; ++y)
        c.hline(0, c.width() - 1, y, lerpColor(skyTop, skyBottom, float(y - top) / (bottom - top)));
}

void drawSky(Canvas& c, int top, int bottom) { drawSkyColors(c, top, bottom, {86, 146, 226}, {196, 224, 248}); }

void drawCloud(Canvas& c, int x, int y, int s) {
    Color shadow{214, 226, 240}, white{250, 252, 255};
    fillEllipse(c, x, y + s / 4, int(s * 1.6f), int(s * 0.45f), shadow);
    fillEllipse(c, x - int(s * 0.8f), y + s / 6, int(s * 0.7f), int(s * 0.42f), white);
    fillEllipse(c, x + int(s * 0.8f), y + s / 8, int(s * 0.75f), int(s * 0.45f), white);
    fillEllipse(c, x, y, s, int(s * 0.6f), white);
}

void drawSun(Canvas& c, int x, int y, int r) {
    drawCircle(c, x, y, r + 14, {255, 246, 206});
    drawCircle(c, x, y, r + 8, {255, 236, 160});
    fillCircle(c, x, y, r, {255, 214, 84});
    fillCircle(c, x - r / 4, y - r / 4, r / 2, {255, 232, 140});
}

void drawHills(Canvas& c, int baseY, float scrollX, Color tint) {
    // Two layers of Bezier hills; the far layer is blended toward the sky
    // colour (atmospheric perspective). Each layer repeats every 840 px so it
    // can scroll for parallax.
    const float b = float(baseY);
    std::vector<Vec2> far = {{-20, b}, {120, b - 120}, {260, b - 10}, {420, b - 140}, {600, b - 20}, {820, b}};
    std::vector<Vec2> near = {{-20, b + 20}, {200, b - 80}, {500, b + 30}, {820, b + 20}};
    for (int layer = 0; layer < 2; ++layer) {
        float speed = layer == 0 ? 0.4f : 1.0f;   // far layer moves slower
        float offset = std::fmod(scrollX * speed, 840.0f);
        Color col = layer == 0 ? lerpColor({70, 140, 90}, {170, 210, 230}, 0.55f) : Color{86, 160, 92};
        col = rgb(col.r * tint.r / 255, col.g * tint.g / 255, col.b * tint.b / 255);
        for (int copy = -1; copy <= 1 + c.width() / 840; ++copy) {
            Polygon hill = sampleBezier(layer == 0 ? far : near, 60);
            hill.push_back({820, float(c.height())});
            hill.push_back({-20, float(c.height())});
            fillPolygonScanline(c, translated(hill, {copy * 840.0f - offset, 0}), col);
        }
    }
}

Canvas renderConcept() {
    Canvas c(W, H);
    drawSky(c, 0, H);
    drawSun(c, 520, 70, 30);
    drawCloud(c, 150, 120, 30);
    drawCloud(c, 360, 70, 22);
    drawCloud(c, 470, 165, 16);
    drawHills(c, 330);

    VoxelWorld world = makeWorld();
    IsoView view{400, 250, 18};
    drawWorldIso(c, view, world);

    const int sx = 9, sz = 11, sy = world.surfaceHeight(sx, sz);
    drawSelection(c, view, sx, sy, sz, {255, 255, 255});
    Vec2 aim = isoProject(view, sx + 0.5f, sy + 1.0f, sz + 0.5f);
    drawCrosshair(c, iround(aim.x), iround(aim.y));

    drawMinimap(c, world, 636, 22, 9, sx + 0.5f, sz + 0.5f, -1.55f);
    drawHotbar(c, 400, H - 66, 0);

    drawText(c, 18, 18, "VOXEL WORLD BUILDER", {20, 30, 60}, 3);
    drawText(c, 16, 16, "VOXEL WORLD BUILDER", {255, 255, 255}, 3);
    drawText(c, 16, 48, "SELECTED: " + std::string(blockName(world.get(sx, sy, sz))) + " AT (" +
                            std::to_string(sx) + "," + std::to_string(sy) + "," + std::to_string(sz) + ")",
             {30, 40, 70}, 2);
    return c;
}

}  // namespace demo
