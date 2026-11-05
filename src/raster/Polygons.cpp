#include "raster/Polygons.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <stack>
#include <utility>

#include "raster/Lines.h"

void drawPolygonOutline(Canvas& canvas, const Polygon& poly, Color c) {
    for (size_t i = 0; i < poly.size(); ++i) {
        const Vec2& a = poly[i];
        const Vec2& b = poly[(i + 1) % poly.size()];
        drawLineBresenham(canvas, iround(a.x), iround(a.y), iround(b.x), iround(b.y), c);
    }
}

namespace {

// One entry of the edge table. x is the intersection with the current
// scanline and is advanced by invSlope (= dx/dy) every scanline.
struct Edge {
    int yEnd;          // first scanline the edge no longer covers
    float x;
    float invSlope;
};

}  // namespace

void fillPolygonScanline(Canvas& canvas, const Polygon& poly, Color c) {
    if (poly.size() < 3) return;

    // 1. Build the Edge Table, bucketed by the first scanline each edge crosses.
    //    Scanlines are sampled at pixel centres (y + 0.5); horizontal edges are skipped.
    std::map<int, std::vector<Edge>> edgeTable;
    for (size_t i = 0; i < poly.size(); ++i) {
        Vec2 a = poly[i], b = poly[(i + 1) % poly.size()];
        if (a.y > b.y) std::swap(a, b);
        int yStart = static_cast<int>(std::ceil(a.y - 0.5f));
        int yEnd = static_cast<int>(std::ceil(b.y - 0.5f));
        if (yStart >= yEnd) continue;
        float invSlope = (b.x - a.x) / (b.y - a.y);
        float x = a.x + (yStart + 0.5f - a.y) * invSlope;
        edgeTable[yStart].push_back({yEnd, x, invSlope});
    }
    if (edgeTable.empty()) return;

    // 2. Walk the scanlines, keeping the Active Edge Table up to date.
    std::vector<Edge> active;
    int y = edgeTable.begin()->first;
    while (!active.empty() || edgeTable.lower_bound(y) != edgeTable.end()) {
        auto bucket = edgeTable.find(y);
        if (bucket != edgeTable.end())
            active.insert(active.end(), bucket->second.begin(), bucket->second.end());
        active.erase(std::remove_if(active.begin(), active.end(),
                                    [y](const Edge& e) { return e.yEnd <= y; }),
                     active.end());
        std::sort(active.begin(), active.end(),
                  [](const Edge& l, const Edge& r) { return l.x < r.x; });

        // 3. Fill between pairs of intersections (even-odd rule).
        for (size_t i = 0; i + 1 < active.size(); i += 2) {
            int xs = static_cast<int>(std::ceil(active[i].x - 0.5f));
            int xe = static_cast<int>(std::ceil(active[i + 1].x - 0.5f)) - 1;
            if (xs <= xe) canvas.hline(xs, xe, y, c);
        }
        for (Edge& e : active) e.x += e.invSlope;
        ++y;
    }
}

void boundaryFill(Canvas& canvas, int x, int y, Color fill, Color boundary) {
    std::stack<std::pair<int, int>> todo;
    todo.push({x, y});
    while (!todo.empty()) {
        auto [px, py] = todo.top();
        todo.pop();
        if (!canvas.inBounds(px, py)) continue;
        Color cur = canvas.getPixel(px, py);
        if (cur == boundary || cur == fill) continue;
        canvas.setPixel(px, py, fill);
        todo.push({px + 1, py}); todo.push({px - 1, py});
        todo.push({px, py + 1}); todo.push({px, py - 1});
    }
}

void floodFill(Canvas& canvas, int x, int y, Color newColor) {
    if (!canvas.inBounds(x, y)) return;
    Color target = canvas.getPixel(x, y);
    if (target == newColor) return;
    std::stack<std::pair<int, int>> todo;
    todo.push({x, y});
    while (!todo.empty()) {
        auto [px, py] = todo.top();
        todo.pop();
        if (!canvas.inBounds(px, py) || canvas.getPixel(px, py) != target) continue;
        canvas.setPixel(px, py, newColor);
        todo.push({px + 1, py}); todo.push({px - 1, py});
        todo.push({px, py + 1}); todo.push({px, py - 1});
    }
}

namespace {

enum class Side { Left, Right, Top, Bottom };

bool inside(Vec2 p, Side s, const ClipRect& w) {
    switch (s) {
        case Side::Left:   return p.x >= w.xmin;
        case Side::Right:  return p.x <= w.xmax;
        case Side::Top:    return p.y >= w.ymin;
        case Side::Bottom: return p.y <= w.ymax;
    }
    return false;
}

Vec2 intersect(Vec2 a, Vec2 b, Side s, const ClipRect& w) {
    float t = 0;
    switch (s) {
        case Side::Left:   t = (w.xmin - a.x) / (b.x - a.x); break;
        case Side::Right:  t = (w.xmax - a.x) / (b.x - a.x); break;
        case Side::Top:    t = (w.ymin - a.y) / (b.y - a.y); break;
        case Side::Bottom: t = (w.ymax - a.y) / (b.y - a.y); break;
    }
    return lerp(a, b, t);
}

}  // namespace

Polygon clipPolygonSutherlandHodgman(const Polygon& poly, const ClipRect& window) {
    Polygon output = poly;
    for (Side side : {Side::Left, Side::Right, Side::Top, Side::Bottom}) {
        Polygon input = output;
        output.clear();
        if (input.empty()) break;
        Vec2 prev = input.back();
        for (const Vec2& cur : input) {
            bool curIn = inside(cur, side, window), prevIn = inside(prev, side, window);
            if (curIn) {
                if (!prevIn) output.push_back(intersect(prev, cur, side, window));  // entering
                output.push_back(cur);
            } else if (prevIn) {
                output.push_back(intersect(prev, cur, side, window));               // leaving
            }
            prev = cur;
        }
    }
    return output;
}

float signedArea(const Polygon& poly) {
    float sum = 0;
    for (size_t i = 0; i < poly.size(); ++i) {
        const Vec2& a = poly[i];
        const Vec2& b = poly[(i + 1) % poly.size()];
        sum += a.x * b.y - b.x * a.y;
    }
    return sum * 0.5f;
}

Vec2 centroid(const Polygon& poly) {
    float a = signedArea(poly);
    if (std::fabs(a) < 1e-6f) {
        Vec2 avg;
        for (const Vec2& p : poly) avg = avg + p;
        return poly.empty() ? avg : avg * (1.0f / poly.size());
    }
    Vec2 c;
    for (size_t i = 0; i < poly.size(); ++i) {
        const Vec2& p = poly[i];
        const Vec2& q = poly[(i + 1) % poly.size()];
        float cross = p.x * q.y - q.x * p.y;
        c.x += (p.x + q.x) * cross;
        c.y += (p.y + q.y) * cross;
    }
    return c * (1.0f / (6.0f * a));
}

bool isConvex(const Polygon& poly) {
    if (poly.size() < 3) return false;
    int sign = 0;
    for (size_t i = 0; i < poly.size(); ++i) {
        Vec2 a = poly[i], b = poly[(i + 1) % poly.size()], c = poly[(i + 2) % poly.size()];
        float cross = (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
        int s = (cross > 0) - (cross < 0);
        if (s == 0) continue;
        if (sign == 0) sign = s;
        else if (s != sign) return false;
    }
    return true;
}

bool pointInPolygon(const Polygon& poly, Vec2 p) {
    bool in = false;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        const Vec2& a = poly[i];
        const Vec2& b = poly[j];
        if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)
            in = !in;
    }
    return in;
}

Polygon regularPolygon(Vec2 center, float radius, int sides, float rotation) {
    Polygon poly;
    for (int i = 0; i < sides; ++i) {
        float a = rotation + 2.0f * 3.14159265f * i / sides;
        poly.push_back({center.x + radius * std::cos(a), center.y + radius * std::sin(a)});
    }
    return poly;
}

Polygon translated(const Polygon& poly, Vec2 offset) {
    Polygon out;
    for (const Vec2& p : poly) out.push_back(p + offset);
    return out;
}
