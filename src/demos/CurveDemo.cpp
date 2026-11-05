// 06 Bezier curves and splines.
#include <cmath>

#include "core/Font.h"
#include "demos/Demos.h"
#include "raster/Circles.h"
#include "raster/Curves.h"
#include "raster/Lines.h"
#include "raster/Polygons.h"

namespace demo {

Canvas curves() {
    Canvas c = makePage("06  CURVES: BEZIER AND SPLINES", "TERRAIN SILHOUETTES, A RIVER AND A CAMERA FLY-THROUGH");

    // A: three layers of Bezier hills, clipped to the panel and filled.
    const ClipRect top{11, 77, 788, 335};
    panel(c, 10, 76, 780, 260, "");
    for (int y = 77; y <= 335; ++y)
        c.hline(11, 788, y, lerpColor({96, 152, 226}, {200, 226, 248}, (y - 77) / 258.0f));
    const std::vector<Vec2> layers[3] = {
        {{0, 230}, {130, 120}, {300, 250}, {460, 110}, {640, 240}, {800, 150}},
        {{0, 280}, {180, 170}, {420, 300}, {620, 160}, {800, 260}},
        {{12, 312}, {220, 170}, {520, 330}, {788, 228}}};
    const Color sky{180, 214, 242};
    for (int i = 0; i < 3; ++i) {
        Polygon hill;
        if (layers[i].size() == 4)   // cubic: Bernstein polynomial form
            for (int s = 0; s <= 80; ++s)
                hill.push_back(cubicBezier(layers[i][0], layers[i][1], layers[i][2], layers[i][3], s / 80.0f));
        else                         // any degree: de Casteljau
            hill = sampleBezier(layers[i], 80);
        hill.push_back({800, 340});
        hill.push_back({0, 340});
        Color col = lerpColor({66, 150, 76}, sky, 0.55f - i * 0.27f);   // distant hills fade into the sky
        fillPolygonScanline(c, clipPolygonSutherlandHodgman(hill, top), col);
    }
    // de Casteljau construction on the front hill at t = 0.4.
    const std::vector<Vec2>& ctrl = layers[2];
    drawControlPolygon(c, ctrl, {255, 255, 255}, ACCENT);
    std::vector<Vec2> level = ctrl;
    const Color levelCol[] = {{255, 140, 60}, {235, 90, 90}};
    for (int l = 0; level.size() > 1; ++l) {
        std::vector<Vec2> next;
        for (size_t i = 0; i + 1 < level.size(); ++i) next.push_back(lerp(level[i], level[i + 1], 0.4f));
        if (next.size() > 1) drawPolyline(c, next, levelCol[l % 2]);
        for (Vec2 p : next) fillCircle(c, iround(p.x), iround(p.y), 3, levelCol[l % 2]);
        level = next;
    }
    fillCircle(c, iround(level[0].x), iround(level[0].y), 6, BAD);
    drawCircle(c, iround(level[0].x), iround(level[0].y), 6, {255, 255, 255});
    label(c, 20, 86, "BEZIER HILLS", {24, 40, 80});
    label(c, 20, 106, "DE CASTELJAU AT T=0.4", {24, 40, 80});

    // B: same points, interpolating vs approximating spline.
    panel(c, 10, 346, 385, 244, "CATMULL-ROM VS B-SPLINE");
    c.fillRect(20, 374, 365, 186, {74, 140, 70});
    std::vector<Vec2> river = {{30, 420}, {100, 520}, {170, 400}, {240, 540}, {300, 430}, {370, 500}};
    drawPolyline(c, sampleCatmullRom(river, 20), {70, 130, 230}, 7);
    drawPolyline(c, sampleBSpline(river, 20), ACCENT, 3);
    for (Vec2 p : river) { fillCircle(c, iround(p.x), iround(p.y), 4, TEXT); drawCircle(c, iround(p.x), iround(p.y), 4, {20, 20, 20}); }
    label(c, 20, 566, "BLUE CATMULL", {120, 170, 255});
    label(c, 176, 566, "/ GOLD B-SPLINE", ACCENT);

    // C: a B-spline camera path around a small world, cameras placed along it.
    panel(c, 405, 346, 385, 244, "CAMERA FLY-THROUGH PATH");
    VoxelWorld world = makeWorld();
    drawWorldIso(c, IsoView{598, 420, 6}, world, nullptr, false);
    std::vector<Vec2> path = {{440, 540}, {460, 400}, {600, 380}, {760, 420}, {740, 540}, {600, 560}, {470, 520}};
    std::vector<Vec2> pts = sampleBSpline(path, 16);
    drawControlPolygon(c, path, MUTED, MUTED);
    drawPolyline(c, pts, TEXT, 2);
    for (size_t i = 6; i + 1 < pts.size(); i += 14) {
        Vec2 p = pts[i], dir = pts[i + 1] - pts[i];
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len == 0) continue;
        dir = dir * (1.0f / len);
        Vec2 n{-dir.y, dir.x};
        Polygon cam = {p + dir * 11.0f, p - dir * 7.0f + n * 7.0f, p - dir * 7.0f - n * 7.0f};
        fillPolygonScanline(c, cam, BAD);
        drawPolygonOutline(c, cam, {255, 255, 255});
    }
    label(c, 415, 566, "RED = CAMERA KEY POSES", MUTED);
    return c;
}

}  // namespace demo
