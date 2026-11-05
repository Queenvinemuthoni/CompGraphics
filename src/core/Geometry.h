#pragma once
// Basic 2D geometry types shared by all raster algorithms.
#include <cmath>
#include <vector>

struct Vec2 {
    float x = 0, y = 0;
};

inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator*(Vec2 a, float s) { return {a.x * s, a.y * s}; }
inline Vec2 lerp(Vec2 a, Vec2 b, float t) { return a + (b - a) * t; }
inline int iround(float v) { return static_cast<int>(std::lround(v)); }

// Axis-aligned clip window (inclusive bounds), used by line and polygon clipping.
struct ClipRect {
    float xmin, ymin, xmax, ymax;
};

using Polygon = std::vector<Vec2>;
