#include "ui/Hud.h"

#include <cmath>
#include <vector>

#include "color/ColorOps.h"
#include "core/Font.h"
#include "raster/Circles.h"
#include "raster/Curves.h"
#include "raster/Lines.h"

const Block HOTBAR_BLOCKS[HOTBAR_SIZE] = {Block::Grass, Block::Dirt, Block::Stone, Block::Sand,
                                          Block::Water, Block::Wood, Block::Leaves};

void drawCrosshair(Canvas& c, int x, int y, Color w) {
    drawCircle(c, x, y, 11, w);
    drawLineBresenham(c, x - 18, y, x - 6, y, w);
    drawLineBresenham(c, x + 6, y, x + 18, y, w);
    drawLineBresenham(c, x, y - 18, x, y - 6, w);
    drawLineBresenham(c, x, y + 6, x, y + 18, w);
    fillCircle(c, x, y, 2, w);
}

void drawSelection(Canvas& c, const IsoView& view, int x, int y, int z, Color col) {
    Polygon s = isoBlockSilhouette(view, x, y, z);
    for (size_t i = 0; i < s.size(); ++i) {
        Vec2 a = s[i], b = s[(i + 1) % s.size()];
        drawDashedLine(c, iround(a.x), iround(a.y), iround(b.x), iround(b.y), 5, 3, col);
    }
    // The three inner edges meeting at the front-top corner.
    Vec2 front = isoProject(view, x + 1, y + 1, z + 1);
    for (Vec2 p : {isoProject(view, x + 1, y + 1, z), isoProject(view, x, y + 1, z + 1),
                   isoProject(view, x + 1, y, z + 1)})
        drawDashedLine(c, iround(front.x), iround(front.y), iround(p.x), iround(p.y), 5, 3, col);
}

Canvas makeBlockIcon(Block b, int size, Color key) {
    Canvas icon(size, size, key);
    float t = size * 0.44f;
    IsoView v{size / 2.0f, size / 2.0f, t};
    drawIsoBlock(icon, v, 0, 0, 0, b);
    return icon;
}

void drawHotbar(Canvas& c, int centerX, int y, int selected) {
    const int slot = 46, gap = 6;
    const Color key{255, 0, 255};
    int x0 = centerX - (HOTBAR_SIZE * slot + (HOTBAR_SIZE - 1) * gap) / 2;
    c.fillRect(x0 - 8, y - 8, HOTBAR_SIZE * slot + (HOTBAR_SIZE - 1) * gap + 16, slot + 16, {28, 30, 40});
    for (int i = 0; i < HOTBAR_SIZE; ++i) {
        int x = x0 + i * (slot + gap);
        c.fillRect(x, y, slot, slot, {58, 62, 78});
        Canvas icon = makeBlockIcon(HOTBAR_BLOCKS[i], slot - 8, key);
        c.blit(icon, x + 4, y + 4, &key);   // colour-key transparency
        Color frame = i == selected ? Color{255, 255, 255} : Color{96, 100, 120};
        drawRectOutline(c, {float(x), float(y), float(x + slot - 1), float(y + slot - 1)}, frame);
        if (i == selected) {
            drawRectOutline(c, {float(x - 2), float(y - 2), float(x + slot + 1), float(y + slot + 1)}, frame);
            fillCircle(c, x + slot / 2, y + slot + 12, 3, frame);
        }
        drawText(c, x + 3, y + 3, std::to_string(i + 1), {230, 230, 240}, 1);
    }
    std::string name = blockName(HOTBAR_BLOCKS[selected]);
    drawShadowText(c, centerX - textWidth(name) / 2, y - 30, name, {255, 255, 255});
}

void drawMinimap(Canvas& c, const VoxelWorld& world, int mx, int my, int cell, float px, float pz,
                 float heading, bool riverOverlay) {
    int sizeX = world.sizeX() * cell, sizeZ = world.sizeZ() * cell;
    c.fillRect(mx - 6, my - 6, sizeX + 12, sizeZ + 12, {20, 22, 30});
    for (int x = 0; x < world.sizeX(); ++x)
        for (int z = 0; z < world.sizeZ(); ++z) {
            int h = world.surfaceHeight(x, z);
            if (h < 0) { c.fillRect(mx + x * cell, my + z * cell, cell, cell, {10, 10, 14}); continue; }
            Block top = world.get(x, h, z);
            c.fillRect(mx + x * cell, my + z * cell, cell, cell, shade(blockBaseColor(top), 0.55f + 0.07f * h));
        }
    if (riverOverlay) {   // river centre line as a Catmull-Rom spline through sample points
        std::vector<Vec2> river;
        for (int x = 0; x <= world.sizeX(); x += 2)
            river.push_back({mx + x * float(cell), my + (7.5f + 2.5f * std::sin(x * 0.5f)) * cell});
        drawPolyline(c, sampleCatmullRom(river, 8), {170, 210, 255});
    }

    // Field of view: two rays clipped to the minimap with Cohen-Sutherland.
    ClipRect win{float(mx), float(my), float(mx + sizeX - 1), float(my + sizeZ - 1)};
    float cx = mx + px * cell, cy = my + pz * cell;
    for (float a : {heading - 0.45f, heading + 0.45f}) {
        float x0 = cx, y0 = cy, x1 = cx + 400 * std::cos(a), y1 = cy + 400 * std::sin(a);
        if (clipLineCohenSutherland(x0, y0, x1, y1, win))
            drawLineBresenham(c, iround(x0), iround(y0), iround(x1), iround(y1), {255, 196, 64});
    }
    if (cx >= win.xmin && cx <= win.xmax && cy >= win.ymin && cy <= win.ymax) {
        fillCircle(c, iround(cx), iround(cy), 4, {235, 90, 90});
        drawCircle(c, iround(cx), iround(cy), 6, {255, 255, 255});
    }
    drawRectOutline(c, {float(mx - 6), float(my - 6), float(mx + sizeX + 5), float(my + sizeZ + 5)}, {200, 200, 210});
    drawShadowText(c, mx, my + sizeZ + 12, "MINIMAP", {255, 255, 255});
}

void drawShadowText(Canvas& c, int x, int y, const std::string& text, Color col, int scale) {
    drawText(c, x + 1, y + 1, text, {20, 24, 40}, scale);
    drawText(c, x, y, text, col, scale);
}
