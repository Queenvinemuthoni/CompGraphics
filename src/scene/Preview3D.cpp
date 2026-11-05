#include "scene/Preview3D.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "color/ColorOps.h"
#include "scene/Voxel.h"

namespace {

struct ScreenPt { int x, y; };

Vec3 sub(Vec3 a, Vec3 b) { return a - b; }   // Vec3 and its helpers come from math/Transform.h
Vec3 rotateY(Vec3 p, float a) { float s = std::sin(a), c = std::cos(a); return {p.x * c + p.z * s, p.y, -p.x * s + p.z * c}; }
Vec3 rotateX(Vec3 p, float a) { float s = std::sin(a), c = std::cos(a); return {p.x, p.y * c - p.z * s, p.y * s + p.z * c}; }

const float FOV_FACTOR = 400.0f;

// World y points up, screen y points down, so y is negated here.
ScreenPt project(Vec3 p, int w, int h) {
    float z = std::max(p.z, 0.1f);
    return {static_cast<int>(p.x * FOV_FACTOR / z) + w / 2, static_cast<int>(-p.y * FOV_FACTOR / z) + h / 2};
}

float edgeFn(ScreenPt a, ScreenPt b, ScreenPt c) {
    return float(c.x - a.x) * (b.y - a.y) - float(c.y - a.y) * (b.x - a.x);
}

void drawTriangle(Canvas& canvas, std::vector<float>& zbuf, ScreenPt p0, float z0, ScreenPt p1, float z1,
                  ScreenPt p2, float z2, Color c) {
    int minX = std::max(0, std::min({p0.x, p1.x, p2.x}));
    int maxX = std::min(canvas.width() - 1, std::max({p0.x, p1.x, p2.x}));
    int minY = std::max(0, std::min({p0.y, p1.y, p2.y}));
    int maxY = std::min(canvas.height() - 1, std::max({p0.y, p1.y, p2.y}));
    float area = edgeFn(p0, p1, p2);
    if (area == 0) return;
    for (int y = minY; y <= maxY; ++y)
        for (int x = minX; x <= maxX; ++x) {
            ScreenPt p{x, y};
            float w0 = edgeFn(p1, p2, p) / area, w1 = edgeFn(p2, p0, p) / area, w2 = edgeFn(p0, p1, p) / area;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;
            float z = w0 * z0 + w1 * z1 + w2 * z2;
            float& depth = zbuf[static_cast<size_t>(y) * canvas.width() + x];
            if (z < depth) { depth = z; canvas.setPixel(x, y, c); }
        }
}

struct FaceDef {
    int dx, dy, dz;   // neighbour offset this face points towards
    int t1[3], t2[3];
};

}  // namespace

void renderPreview3D(Canvas& canvas) {
    const int W = canvas.width(), H = canvas.height();
    std::vector<float> zbuf(static_cast<size_t>(W) * H, std::numeric_limits<float>::max());

    const Vec3 cube[8] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                          {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};
    const FaceDef faces[6] = {
        {0, 0, -1, {0, 2, 1}, {0, 3, 2}}, {1, 0, 0, {1, 6, 5}, {1, 2, 6}},
        {0, 0, 1, {5, 7, 4}, {5, 6, 7}},  {-1, 0, 0, {4, 3, 0}, {4, 7, 3}},
        {0, 1, 0, {3, 6, 2}, {3, 7, 6}},  {0, -1, 0, {4, 0, 1}, {4, 1, 5}}};

    // world[y][x][z], y = 0 is the bottom layer.
    const Block S = Block::Stone, D = Block::Dirt, G = Block::Grass, A = Block::Air;
    const Block world[3][3][3] = {
        {{S, S, S}, {S, S, S}, {S, S, S}},
        {{D, D, D}, {D, A, D}, {D, D, D}},
        {{G, A, G}, {A, A, A}, {G, A, G}}};
    auto at = [&](int x, int y, int z) {
        return (x < 0 || y < 0 || z < 0 || x > 2 || y > 2 || z > 2) ? A : world[y][x][z];
    };

    const Vec3 toLight = normalize({0.6f, 1.0f, -0.5f});
    const float camYaw = 0.6f, camPitch = -0.55f, camDist = 13.0f;

    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            for (int z = 0; z < 3; ++z) {
                Block b = at(x, y, z);
                if (b == A) continue;
                Color base = blockBaseColor(b);
                // Model -> world: place the cube in the grid (2 units per block), centred on the origin.
                Vec3 offset = {(x - 1) * 2.0f, (y - 1) * 2.0f, (z - 1) * 2.0f};
                for (const FaceDef& f : faces) {
                    if (at(x + f.dx, y + f.dy, z + f.dz) != A) continue;   // hidden-face culling
                    for (const int* tri : {f.t1, f.t2}) {
                        Vec3 v[3];
                        for (int k = 0; k < 3; ++k) {
                            Vec3 p = cube[tri[k]];
                            p = {p.x + offset.x, p.y + offset.y, p.z + offset.z};   // translate first
                            p = rotateX(rotateY(p, camYaw), camPitch);              // then orbit camera
                            p.z += camDist;                                         // then move away
                            v[k] = p;
                        }
                        Vec3 n = normalize(cross(sub(v[1], v[0]), sub(v[2], v[0])));
                        if (dot(n, sub({0, 0, 0}, v[0])) <= 0) continue;           // back-face culling
                        // Lighting uses the world-space normal (rotation-independent).
                        Vec3 wn = {float(f.dx), float(f.dy), float(f.dz)};
                        float intensity = std::min(1.0f, 0.35f + 0.75f * std::max(0.0f, dot(wn, toLight)));
                        Color c = (b == Block::Grass && f.dy != 1) ? blockBaseColor(Block::Dirt) : base;
                        c = shade(c, intensity);
                        drawTriangle(canvas, zbuf, project(v[0], W, H), v[0].z, project(v[1], W, H), v[1].z,
                                     project(v[2], W, H), v[2].z, c);
                    }
                }
            }
}
