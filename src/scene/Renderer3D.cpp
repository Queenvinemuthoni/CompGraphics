#include "scene/Renderer3D.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

struct CubeFace {
    Vec3 corners[4];   // in cyclic order
    Vec3 normal;
};

const CubeFace CUBE_FACES[6] = {
    {{{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}, {1, 0, 0}},
    {{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}, {-1, 0, 0}},
    {{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}, {0, 1, 0}},
    {{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}, {0, -1, 0}},
    {{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}, {0, 0, 1}},
    {{{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}, {0, 0, -1}},
};

Color faceColor(Block b, int face) {
    if (b == Block::Grass && face != 2) return blockBaseColor(Block::Dirt);   // grass only on top
    return blockBaseColor(b);
}

}  // namespace

SoftRenderer::SoftRenderer(Canvas& target)
    : canvas_(target), depth_(static_cast<size_t>(target.width()) * target.height(), 1e30f) {}

void SoftRenderer::setCamera(const Mat4& view, const Mat4& projection, Vec3 eye) {
    viewProj_ = projection * view;
    eye_ = eye;
}

void SoftRenderer::setLight(Vec3 towardsLight, float ambient, Color tint) {
    light_ = normalize(towardsLight);
    ambient_ = ambient;
    tint_ = tint;
}

void SoftRenderer::setFog(Color color, float start, float end) {
    fog_ = color;
    fogStart_ = start;
    fogEnd_ = end;
}

void SoftRenderer::clearDepth() { std::fill(depth_.begin(), depth_.end(), 1e30f); }

void SoftRenderer::drawCube(const Mat4& model, Block b, int faceMask) {
    for (int f = 0; f < 6; ++f) {
        if (!(faceMask & (1 << f))) continue;
        Vec3 world[4];
        for (int k = 0; k < 4; ++k) world[k] = model.applyPoint(CUBE_FACES[f].corners[k]);
        drawFace(world, normalize(model.applyVector(CUBE_FACES[f].normal)), faceColor(b, f));
    }
}

int SoftRenderer::drawWorld(const VoxelWorld& world) {
    static const int dir[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    int faces = 0;
    for (int y = 0; y < world.sizeY(); ++y)
        for (int z = 0; z < world.sizeZ(); ++z)
            for (int x = 0; x < world.sizeX(); ++x) {
                Block b = world.get(x, y, z);
                if (b == Block::Air) continue;
                int mask = 0;
                for (int f = 0; f < 6; ++f) {
                    Block n = world.get(x + dir[f][0], y + dir[f][1], z + dir[f][2]);
                    bool visible = b == Block::Water ? n == Block::Air : !isOpaque(n);
                    if (visible) { mask |= 1 << f; ++faces; }
                }
                if (mask) drawCube(Mat4::translate({float(x), float(y), float(z)}), b, mask);
            }
    return faces;
}

void SoftRenderer::drawFace(const Vec3 corners[4], Vec3 normal, Color base) {
    // Back-face culling in world space: skip faces pointing away from the eye.
    if (dot(normal, eye_ - corners[0]) <= 0) return;

    // Flat shading: ambient + Lambert diffuse, tinted by the light colour, then fog.
    float diffuse = std::max(0.0f, dot(normal, light_));
    float intensity = ambient_ + (1.0f - ambient_) * diffuse;
    Color lit = rgb(int(base.r * intensity * tint_.r / 255), int(base.g * intensity * tint_.g / 255),
                    int(base.b * intensity * tint_.b / 255));
    Vec3 centre = (corners[0] + corners[2]) * 0.5f;
    float fog = std::clamp((length(centre - eye_) - fogStart_) / (fogEnd_ - fogStart_), 0.0f, 1.0f);
    Color c = lerpColor(lit, fog_, fog);

    // To clip space, then clip against the near plane (z >= -w) - Sutherland-Hodgman with one plane.
    std::vector<Vec4> in, out;
    for (int k = 0; k < 4; ++k) in.push_back(viewProj_.apply({corners[k].x, corners[k].y, corners[k].z, 1}));
    for (size_t i = 0; i < in.size(); ++i) {
        Vec4 a = in[i], b = in[(i + 1) % in.size()];
        float da = a.z + a.w, db = b.z + b.w;
        if (da >= 0) out.push_back(a);
        if ((da >= 0) != (db >= 0)) {
            float t = da / (da - db);
            out.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t});
        }
    }
    if (out.size() < 3) return;

    // Perspective divide + viewport transform.
    const float W = float(canvas_.width()), H = float(canvas_.height());
    std::vector<Vec3> screen;
    for (const Vec4& v : out) {
        float w = std::max(v.w, 1e-5f);
        screen.push_back({(v.x / w + 1) * 0.5f * W, (1 - v.y / w) * 0.5f * H, v.z / w});
    }
    for (size_t i = 1; i + 1 < screen.size(); ++i) {   // triangle fan
        Vec3 tri[3] = {screen[0], screen[i], screen[i + 1]};
        rasterTriangle(tri, c);
    }
}

void SoftRenderer::rasterTriangle(const Vec3 s[3], Color c) {
    auto edge = [](Vec3 a, Vec3 b, float px, float py) { return (px - a.x) * (b.y - a.y) - (py - a.y) * (b.x - a.x); };
    float area = edge(s[0], s[1], s[2].x, s[2].y);
    if (std::fabs(area) < 1e-6f) return;
    int minX = std::max(0, int(std::floor(std::min({s[0].x, s[1].x, s[2].x}))));
    int maxX = std::min(canvas_.width() - 1, int(std::ceil(std::max({s[0].x, s[1].x, s[2].x}))));
    int minY = std::max(0, int(std::floor(std::min({s[0].y, s[1].y, s[2].y}))));
    int maxY = std::min(canvas_.height() - 1, int(std::ceil(std::max({s[0].y, s[1].y, s[2].y}))));
    for (int y = minY; y <= maxY; ++y)
        for (int x = minX; x <= maxX; ++x) {
            float px = x + 0.5f, py = y + 0.5f;
            float w0 = edge(s[1], s[2], px, py) / area;
            float w1 = edge(s[2], s[0], px, py) / area;
            float w2 = edge(s[0], s[1], px, py) / area;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;
            float z = w0 * s[0].z + w1 * s[1].z + w2 * s[2].z;
            float& d = depth_[static_cast<size_t>(y) * canvas_.width() + x];
            if (z < d) { d = z; canvas_.setPixel(x, y, c); }
        }
}
