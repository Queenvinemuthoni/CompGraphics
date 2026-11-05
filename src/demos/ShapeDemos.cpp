// 03 Circles and ellipses, 04 Polygon representation and filling, 05 Polygon clipping.
#include <cmath>
#include <cstdio>

#include "color/ColorOps.h"
#include "core/Font.h"
#include "demos/Demos.h"
#include "raster/Circles.h"
#include "raster/Lines.h"
#include "raster/Polygons.h"

namespace demo {

namespace {

std::string fmt1(float v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.0f", v);
    return buf;
}

Polygon star(Vec2 c, float rOut, float rIn, int points) {
    Polygon p;
    for (int i = 0; i < points * 2; ++i) {
        float a = -3.14159265f / 2 + i * 3.14159265f / points;
        float r = i % 2 ? rIn : rOut;
        p.push_back({c.x + r * std::cos(a), c.y + r * std::sin(a)});
    }
    return p;
}

// Builder character: made only of circles and ellipses.
void drawBuilder(Canvas& c, int x, int feetY) {
    fillEllipse(c, x - 7, feetY - 6, 6, 5, {60, 60, 80});          // feet
    fillEllipse(c, x + 7, feetY - 6, 6, 5, {60, 60, 80});
    fillEllipse(c, x, feetY - 30, 15, 22, {60, 110, 200});         // body
    drawEllipse(c, x, feetY - 30, 15, 22, {30, 56, 110});
    fillCircle(c, x, feetY - 62, 13, {240, 196, 150});             // head
    drawCircle(c, x, feetY - 62, 13, {150, 110, 80});
    fillEllipse(c, x, feetY - 72, 14, 6, {255, 200, 40});          // hard hat
    fillCircle(c, x - 5, feetY - 62, 2, {30, 30, 40});             // eyes
    fillCircle(c, x + 5, feetY - 62, 2, {30, 30, 40});
}

void arrow(Canvas& c, int x0, int y, int x1, Color col) {
    drawThickLine(c, x0, y, x1 - 8, y, 3, col);
    fillPolygonScanline(c, {{float(x1 - 12), float(y - 8)}, {float(x1 + 2), float(y)}, {float(x1 - 12), float(y + 8)}}, col);
}

}  // namespace

Canvas circlesEllipses() {
    Canvas c = makePage("03  CIRCLES AND ELLIPSES", "MIDPOINT ALGORITHMS: SUN, CLOUDS, CROSSHAIR, CHARACTER");

    // Sky strip using the scene pieces.
    Canvas sky(780, 262);
    drawSky(sky, 0, 262);
    drawSun(sky, 640, 110, 48);
    drawCloud(sky, 150, 80, 34);
    drawCloud(sky, 360, 60, 22);
    drawCloud(sky, 260, 170, 18);
    drawCrosshair(sky, 470, 160);
    c.blit(sky, 10, 74);
    drawRectOutline(c, {10, 74, 789, 335}, {58, 62, 80});
    Color ink{24, 40, 80};
    label(c, 24, 84, "CLOUDS = FILLED ELLIPSES", ink);
    label(c, 520, 84, "SUN = CIRCLE + RINGS", ink);
    label(c, 400, 250, "CROSSHAIR = CIRCLE + LINES", ink);

    // A: octant colouring shows the 8-way symmetry of the midpoint circle.
    panel(c, 10, 342, 256, 248, "8-WAY SYMMETRY");
    Canvas tiny(33, 33, {30, 33, 44});
    Color oct[8];
    for (int i = 0; i < 8; ++i) oct[i] = hsvToRgb({i * 45.0f, 0.65f, 1.0f});
    midpointCircle(14, [&](int x, int y) {
        const int pts[8][2] = {{x, y}, {y, x}, {y, -x}, {x, -y}, {-x, -y}, {-y, -x}, {-y, x}, {-x, y}};
        for (int k = 0; k < 8; ++k) tiny.setPixel(16 + pts[k][0], 16 + pts[k][1], oct[k]);
    });
    c.blitScaled(tiny, 40, 372, 198, 198);
    label(c, 20, 572, "8 PIXELS PER POINT", MUTED);

    // B: ellipse family - the circle is the special case rx = ry.
    panel(c, 272, 342, 256, 248, "MIDPOINT ELLIPSE");
    for (int i = 0; i < 6; ++i) {
        int ry = 15 + i * 17;
        drawEllipse(c, 400, 474, 105, ry, hsvToRgb({200.0f - i * 25.0f, 0.6f, 1.0f}));
    }
    drawCircle(c, 400, 474, 40, ACCENT);
    label(c, 282, 572, "RX=105 RY=15-100", MUTED);

    // C: the builder character standing on a grass block, with an elliptical shadow.
    panel(c, 534, 342, 256, 248, "CHARACTER + SHADOW");
    IsoView v{662, 470, 58};
    drawIsoBlock(c, v, 0, 0, 0, Block::Grass);
    Vec2 top = isoProject(v, 0.5f, 1, 0.5f);
    fillEllipse(c, iround(top.x), iround(top.y), 26, 11, shade(blockBaseColor(Block::Grass), 0.6f));
    drawBuilder(c, iround(top.x), iround(top.y) + 4);
    label(c, 544, 572, "ONLY ROUND SHAPES", MUTED);
    return c;
}

Canvas polygonFill() {
    Canvas c = makePage("04  POLYGONS AND FILLING", "SCANLINE (ET/AET), BOUNDARY FILL, FLOOD FILL, PROPERTIES");

    // A: scanline fill of a concave cursor and a star, with sample scanlines.
    panel(c, 10, 76, 385, 256, "SCANLINE FILL (ET + AET)");
    Polygon cursor = {{0, 0}, {0, 17}, {4, 13}, {7, 20}, {10, 19}, {7, 12}, {12, 12}};
    for (Vec2& p : cursor) p = p * 9.0f + Vec2{50, 112};
    Polygon st = star({275, 210}, 92, 40, 5);
    fillPolygonScanline(c, cursor, TEXT);
    drawPolygonOutline(c, cursor, {20, 20, 20});
    fillPolygonScanline(c, st, ACCENT);
    drawPolygonOutline(c, st, {150, 100, 20});
    for (int y : {150, 196, 256}) {
        drawDashedLine(c, 20, y, 385, y, 4, 4, {90, 200, 255});
        for (const Polygon* poly : {&cursor, &st})
            for (size_t i = 0; i < poly->size(); ++i) {
                Vec2 a = (*poly)[i], b = (*poly)[(i + 1) % poly->size()];
                float yc = y + 0.5f;
                if ((a.y <= yc) != (b.y <= yc)) {
                    float x = a.x + (yc - a.y) * (b.x - a.x) / (b.y - a.y);
                    fillCircle(c, iround(x), y, 3, BAD);
                }
            }
    }
    label(c, 20, 306, "RED = EDGE CROSSINGS, FILL PAIRS", MUTED);

    // B: seed fills.
    panel(c, 405, 76, 385, 256, "BOUNDARY FILL + FLOOD FILL");
    IsoView hv{470, 160, 44};
    Polygon sil = isoBlockSilhouette(hv, 0, 0, 0);
    drawPolygonOutline(c, sil, BAD);
    Vec2 hs = isoProject(hv, 0.5f, 0.5f, 0.5f);   // seed: centre of the block
    int sx = iround(hs.x), sy = iround(hs.y);
    boundaryFill(c, sx, sy, {255, 220, 110}, BAD);
    drawLineBresenham(c, sx - 6, sy, sx + 6, sy, {20, 20, 20});
    drawLineBresenham(c, sx, sy - 6, sx, sy + 6, {20, 20, 20});
    label(c, sx + 8, sy + 4, "SEED", {20, 20, 20});
    label(c, 420, 276, "BOUNDARY", TEXT);
    label(c, 420, 296, "STOPS AT RED", MUTED);

    IsoView before{620, 160, 40}, after{730, 160, 40};
    drawIsoBlock(c, before, 0, 0, 0, Block::Grass);
    drawIsoBlock(c, after, 0, 0, 0, Block::Grass);
    Vec2 seed = isoProject(after, 0.5f, 1, 0.5f);
    floodFill(c, iround(seed.x), iround(seed.y), {240, 246, 255});   // snow on top
    arrow(c, 660, 196, 690, MUTED);
    label(c, 580, 276, "FLOOD: GRASS->SNOW", TEXT);
    label(c, 580, 296, "SAME-COLOUR REGION", MUTED);

    // C: a block is three face polygons.
    panel(c, 10, 342, 385, 248, "BLOCK = 3 FACE POLYGONS");
    IsoView ev{120, 430, 52};
    BlockFaces f = isoBlockFaces(ev, 0, 0, 0);
    Color g = blockBaseColor(Block::Grass), d = blockBaseColor(Block::Dirt);
    struct { Polygon poly; Color col; Vec2 shift; const char* name; } parts[] = {
        {f.top, g, {0, -22}, "TOP"},
        {f.left, shade(d, 0.8f), {-16, 12}, "LEFT"},
        {f.right, shade(d, 0.62f), {16, 12}, "RIGHT"}};
    for (auto& part : parts) {
        Polygon p = translated(part.poly, part.shift);
        fillPolygonScanline(c, p, part.col);
        drawPolygonOutline(c, p, TEXT);
        for (Vec2 q : p) fillCircle(c, iround(q.x), iround(q.y), 3, ACCENT);
        Vec2 m = centroid(p);
        int ly = part.shift.y < 0 ? iround(m.y) - 7 : iround(m.y) + 50;   // side labels go below the face
        label(c, iround(m.x) - textWidth(part.name) / 2, ly, part.name, part.shift.y < 0 ? Color{20, 20, 30} : TEXT);
    }
    label(c, 238, 466, "=", TEXT, 3);
    drawIsoBlock(c, IsoView{320, 446, 38}, 0, 0, 0, Block::Grass);
    label(c, 20, 572, "4 VERTICES PER FACE", MUTED);

    // D: area, centroid, convexity, point-in-polygon.
    panel(c, 405, 342, 385, 248, "POLYGON PROPERTIES");
    Polygon hex = regularPolygon({500, 460}, 62, 6);
    Polygon st2 = star({690, 460}, 72, 32, 5);
    fillPolygonScanline(c, hex, {70, 110, 170});
    fillPolygonScanline(c, st2, {70, 110, 170});
    for (int y = 386; y <= 535; y += 12)
        for (int x = 616; x <= 766; x += 12)
            fillCircle(c, x, y, 2, pointInPolygon(st2, {float(x), float(y)}) ? GOOD : BAD);
    for (const Polygon* p : {&hex, &st2}) {
        Vec2 m = centroid(*p);
        drawLineBresenham(c, iround(m.x) - 6, iround(m.y), iround(m.x) + 6, iround(m.y), TEXT);
        drawLineBresenham(c, iround(m.x), iround(m.y) - 6, iround(m.x), iround(m.y) + 6, TEXT);
    }
    label(c, 420, 540, "AREA " + fmt1(std::fabs(signedArea(hex))), TEXT);
    label(c, 420, 560, std::string("CONVEX: ") + (isConvex(hex) ? "YES" : "NO"), GOOD);
    label(c, 610, 540, "AREA " + fmt1(std::fabs(signedArea(st2))), TEXT);
    label(c, 610, 560, std::string("CONVEX: ") + (isConvex(st2) ? "YES" : "NO"), BAD);
    return c;
}

Canvas polygonClipping() {
    Canvas c = makePage("05  POLYGON CLIPPING", "SUTHERLAND-HODGMAN: THE WORLD CLIPPED TO A VIEWPORT");

    // Left: the whole world faded, then redrawn with every face clipped to the window.
    VoxelWorld world = makeWorld();
    IsoView view{272, 268, 15};
    const ClipRect win{120, 190, 420, 420};
    drawWorldIso(c, view, world);
    for (int y = 70; y < H; ++y)
        for (int x = 0; x < 540; ++x) c.setPixel(x, y, lerpColor(c.getPixel(x, y), BG, 0.72f));
    WorldDrawStats stats = drawWorldIso(c, view, world, &win);
    drawRectOutline(c, win, TEXT);
    drawRectOutline(c, {win.xmin - 1, win.ymin - 1, win.xmax + 1, win.ymax + 1}, TEXT);
    label(c, 124, 172, "CLIP WINDOW", TEXT);
    label(c, 16, 548, std::to_string(stats.facesDrawn) + " FACES CLIPPED BEFORE FILLING", MUTED);
    label(c, 16, 572, "FADED = OUTSIDE THE WINDOW", MUTED);

    // Right: one polygon clipped edge by edge.
    const char* names[] = {"1. LEFT", "2. RIGHT", "3. TOP", "4. BOTTOM"};
    const float BIG = 1e9f;
    for (int k = 0; k < 4; ++k) {
        int px = 548, py = 76 + k * 128;
        panel(c, px, py, 242, 124, "");
        ClipRect w{float(px + 70), float(py + 36), float(px + 172), float(py + 104)};
        Polygon poly = regularPolygon({float(px + 116), float(py + 68)}, 60, 6, 0.3f);
        // Apply only the first k+1 clip edges.
        ClipRect stage{k >= 0 ? w.xmin : -BIG, k >= 2 ? w.ymin : -BIG, k >= 1 ? w.xmax : BIG, k >= 3 ? w.ymax : BIG};
        Polygon out = clipPolygonSutherlandHodgman(poly, stage);
        for (size_t i = 0; i < poly.size(); ++i) {
            Vec2 a = poly[i], b = poly[(i + 1) % poly.size()];
            drawDashedLine(c, iround(a.x), iround(a.y), iround(b.x), iround(b.y), 3, 3, MUTED);
        }
        fillPolygonScanline(c, out, {72, 150, 90});
        drawPolygonOutline(c, out, TEXT);
        for (Vec2 q : out) fillCircle(c, iround(q.x), iround(q.y), 2, ACCENT);
        drawRectOutline(c, w, {90, 200, 255});
        label(c, px + 6, py + 6, names[k], ACCENT);
        label(c, px + 160, py + 6, "V=" + std::to_string(out.size()), TEXT);
    }
    return c;
}

}  // namespace demo
