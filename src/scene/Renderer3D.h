#pragma once
// Software 3D renderer for the voxel world (CP2): the full viewing pipeline on
// the CPU, using the Mat4 transformations.
//   model -> world -> view (look-at) -> clip (perspective) -> near-plane clip
//   -> perspective divide -> viewport -> back-face cull -> z-buffered triangles
// Faces are flat shaded (ambient + Lambert diffuse) and fade into fog.
#include <vector>

#include "core/Canvas.h"
#include "math/Transform.h"
#include "scene/Voxel.h"

class SoftRenderer {
public:
    explicit SoftRenderer(Canvas& target);

    void setCamera(const Mat4& view, const Mat4& projection, Vec3 eye);
    void setLight(Vec3 towardsLight, float ambient, Color tint = {255, 255, 255});
    void setFog(Color color, float start, float end);

    void clearDepth();
    // Unit cube [0,1]^3 transformed by `model`. faceMask bits: 1 +x, 2 -x, 4 +y, 8 -y, 16 +z, 32 -z.
    void drawCube(const Mat4& model, Block b, int faceMask = 63);
    // Every block with hidden-face culling. Returns the number of faces sent to the pipeline.
    int drawWorld(const VoxelWorld& world);

private:
    void drawFace(const Vec3 corners[4], Vec3 normal, Color base);
    void rasterTriangle(const Vec3 s[3], Color c);

    Canvas& canvas_;
    std::vector<float> depth_;
    Mat4 viewProj_;
    Vec3 eye_;
    Vec3 light_ = normalize({0.4f, 1.0f, 0.3f});
    float ambient_ = 0.4f;
    Color tint_{255, 255, 255};
    Color fog_{200, 220, 240};
    float fogStart_ = 1e9f, fogEnd_ = 2e9f;
};
