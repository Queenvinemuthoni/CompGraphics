#include "raster/Curves.h"

#include <algorithm>

#include "raster/Circles.h"
#include "raster/Lines.h"

Vec2 deCasteljau(const std::vector<Vec2>& ctrl, float t) {
    // Repeated linear interpolation until a single point remains.
    std::vector<Vec2> pts = ctrl;
    for (size_t level = pts.size(); level > 1; --level)
        for (size_t i = 0; i + 1 < level; ++i) pts[i] = lerp(pts[i], pts[i + 1], t);
    return pts.empty() ? Vec2{} : pts[0];
}

Vec2 cubicBezier(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t) {
    // B(t) = (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t) t^2 P2 + t^3 P3
    float u = 1.0f - t;
    float b0 = u * u * u, b1 = 3 * u * u * t, b2 = 3 * u * t * t, b3 = t * t * t;
    return p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;
}

std::vector<Vec2> sampleBezier(const std::vector<Vec2>& ctrl, int segments) {
    std::vector<Vec2> out;
    for (int i = 0; i <= segments; ++i) out.push_back(deCasteljau(ctrl, float(i) / segments));
    return out;
}

std::vector<Vec2> sampleCatmullRom(const std::vector<Vec2>& points, int segmentsPerSpan) {
    std::vector<Vec2> out;
    int n = static_cast<int>(points.size());
    if (n < 2) return points;
    auto at = [&](int i) { return points[std::clamp(i, 0, n - 1)]; };  // repeat end points
    for (int i = 0; i < n - 1; ++i) {
        Vec2 p0 = at(i - 1), p1 = at(i), p2 = at(i + 1), p3 = at(i + 2);
        for (int s = (i == 0 ? 0 : 1); s <= segmentsPerSpan; ++s) {
            float t = float(s) / segmentsPerSpan, t2 = t * t, t3 = t2 * t;
            // 0.5 * [ 2P1 + (-P0+P2)t + (2P0-5P1+4P2-P3)t^2 + (-P0+3P1-3P2+P3)t^3 ]
            Vec2 p = (p1 * 2.0f + (p2 - p0) * t + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
                      (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;
            out.push_back(p);
        }
    }
    return out;
}

std::vector<Vec2> sampleBSpline(const std::vector<Vec2>& ctrl, int segmentsPerSpan) {
    // Repeat the end points so the curve starts and ends on them.
    std::vector<Vec2> p;
    if (ctrl.empty()) return p;
    p.push_back(ctrl.front()); p.push_back(ctrl.front());
    p.insert(p.end(), ctrl.begin(), ctrl.end());
    p.push_back(ctrl.back()); p.push_back(ctrl.back());

    std::vector<Vec2> out;
    for (size_t i = 0; i + 3 < p.size(); ++i) {
        for (int s = (i == 0 ? 0 : 1); s <= segmentsPerSpan; ++s) {
            float t = float(s) / segmentsPerSpan, u = 1 - t;
            // Uniform cubic B-spline basis functions (they sum to 1).
            float b0 = u * u * u / 6.0f;
            float b1 = (3 * t * t * t - 6 * t * t + 4) / 6.0f;
            float b2 = (-3 * t * t * t + 3 * t * t + 3 * t + 1) / 6.0f;
            float b3 = t * t * t / 6.0f;
            out.push_back(p[i] * b0 + p[i + 1] * b1 + p[i + 2] * b2 + p[i + 3] * b3);
        }
    }
    return out;
}

void drawPolyline(Canvas& canvas, const std::vector<Vec2>& pts, Color c, int thickness) {
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        int x0 = iround(pts[i].x), y0 = iround(pts[i].y);
        int x1 = iround(pts[i + 1].x), y1 = iround(pts[i + 1].y);
        if (thickness <= 1) drawLineBresenham(canvas, x0, y0, x1, y1, c);
        else drawThickLine(canvas, x0, y0, x1, y1, thickness, c);
    }
}

void drawControlPolygon(Canvas& canvas, const std::vector<Vec2>& ctrl, Color lineColor, Color pointColor) {
    for (size_t i = 0; i + 1 < ctrl.size(); ++i)
        drawDashedLine(canvas, iround(ctrl[i].x), iround(ctrl[i].y),
                       iround(ctrl[i + 1].x), iround(ctrl[i + 1].y), 5, 4, lineColor);
    for (const Vec2& p : ctrl) {
        fillCircle(canvas, iround(p.x), iround(p.y), 4, pointColor);
        drawCircle(canvas, iround(p.x), iround(p.y), 4, {20, 20, 20});
    }
}
