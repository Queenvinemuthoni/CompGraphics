#pragma once
// Voxel ray casting (Amanatides & Woo grid traversal): steps exactly from one
// cell boundary to the next, so it can never skip a block, and reports which
// face was entered so a new block can be placed against it.
#include "math/Transform.h"
#include "scene/Voxel.h"

struct RayHit {
    bool hit = false;
    int x = 0, y = 0, z = 0;      // the block that was hit
    int nx = 0, ny = 0, nz = 0;   // normal of the face that was hit
    float distance = 0;
};

// Water is passed through unless hitWater is true (used to scoop water up).
RayHit raycastVoxels(const VoxelWorld& world, Vec3 origin, Vec3 direction, float maxDistance, bool hitWater = false);
