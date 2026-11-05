#pragma once
// Curves: Bezier curves and splines.
//   - Bezier of any degree via de Casteljau's algorithm
//   - Cubic Bezier via the Bernstein polynomial form
//   - Catmull-Rom spline (interpolating: passes through every point)
//   - Uniform cubic B-spline (approximating: smooth, follows the control polygon)
// Curves are sampled into polylines which are then rasterised with Bresenham.
#include <vector>

#include "core/Canvas.h"
#include "core/Geometry.h"

Vec2 deCasteljau(const std::vector<Vec2>& ctrl, float t);
Vec2 cubicBezier(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, float t);

std::vector<Vec2> sampleBezier(const std::vector<Vec2>& ctrl, int segments);
std::vector<Vec2> sampleCatmullRom(const std::vector<Vec2>& points, int segmentsPerSpan);
std::vector<Vec2> sampleBSpline(const std::vector<Vec2>& ctrl, int segmentsPerSpan);

void drawPolyline(Canvas& canvas, const std::vector<Vec2>& pts, Color c, int thickness = 1);
// Dashed control polygon with a small marker on every control point.
void drawControlPolygon(Canvas& canvas, const std::vector<Vec2>& ctrl, Color lineColor, Color pointColor);
