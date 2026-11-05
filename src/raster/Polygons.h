#pragma once
// Polygon representation, filling and clipping.
//   - Representation: ordered vertex list (Polygon = std::vector<Vec2>)
//   - Properties: signed area (shoelace), centroid, convexity, point-in-polygon
//   - Filling: scanline fill with Edge Table / Active Edge Table,
//              boundary fill and flood fill (4-connected seed fills)
//   - Clipping: Sutherland-Hodgman against a rectangular window
#include "core/Canvas.h"
#include "core/Geometry.h"

void drawPolygonOutline(Canvas& canvas, const Polygon& poly, Color c);
void fillPolygonScanline(Canvas& canvas, const Polygon& poly, Color c);

// Seed fills. Both use an explicit stack instead of recursion so large regions
// do not overflow the call stack.
void boundaryFill(Canvas& canvas, int x, int y, Color fill, Color boundary);
void floodFill(Canvas& canvas, int x, int y, Color newColor);

Polygon clipPolygonSutherlandHodgman(const Polygon& poly, const ClipRect& window);

float signedArea(const Polygon& poly);   // > 0 for clockwise on screen (y down)
Vec2 centroid(const Polygon& poly);
bool isConvex(const Polygon& poly);
bool pointInPolygon(const Polygon& poly, Vec2 p);   // even-odd ray casting
Polygon regularPolygon(Vec2 center, float radius, int sides, float rotation = 0.0f);
Polygon translated(const Polygon& poly, Vec2 offset);
