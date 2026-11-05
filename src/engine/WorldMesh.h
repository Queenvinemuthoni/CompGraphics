#pragma once
// Builds GPU meshes from the voxel world. Only faces next to a non-opaque
// neighbour are emitted (hidden-face culling, the same rule as the pure C++
// renderers), and the whole world becomes two draw calls: opaque + water.
#include <functional>

#include "engine/GlUtil.h"
#include "scene/Voxel.h"

class WorldMesh {
public:
    // skip(x, y, z) lets the engine leave out blocks that are being animated.
    void rebuild(const VoxelWorld& world, const std::function<bool(int, int, int)>& skip);
    void drawOpaque() const { opaque_.draw(); }
    void drawWater() const { water_.draw(); }

private:
    Mesh opaque_, water_;
};

// Colour of one face of a block (grass shows dirt on its sides and bottom).
Color blockFaceColor(Block b, int face);
