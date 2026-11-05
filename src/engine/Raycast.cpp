#include "engine/Raycast.h"

#include <cmath>
#include <limits>

RayHit raycastVoxels(const VoxelWorld& world, Vec3 o, Vec3 d, float maxDistance) {
    d = normalize(d);
    int x = int(std::floor(o.x)), y = int(std::floor(o.y)), z = int(std::floor(o.z));
    const int stepX = d.x > 0 ? 1 : -1, stepY = d.y > 0 ? 1 : -1, stepZ = d.z > 0 ? 1 : -1;
    const float inf = std::numeric_limits<float>::infinity();
    // Distance along the ray between two x (y, z) boundaries, and to the first one.
    const float deltaX = d.x != 0 ? std::fabs(1 / d.x) : inf;
    const float deltaY = d.y != 0 ? std::fabs(1 / d.y) : inf;
    const float deltaZ = d.z != 0 ? std::fabs(1 / d.z) : inf;
    float tMaxX = d.x != 0 ? ((stepX > 0 ? x + 1 - o.x : o.x - x) * deltaX) : inf;
    float tMaxY = d.y != 0 ? ((stepY > 0 ? y + 1 - o.y : o.y - y) * deltaY) : inf;
    float tMaxZ = d.z != 0 ? ((stepZ > 0 ? z + 1 - o.z : o.z - z) * deltaZ) : inf;

    RayHit hit;
    float t = 0;
    while (t <= maxDistance) {
        Block b = world.get(x, y, z);
        if (b != Block::Air && b != Block::Water) {
            hit.hit = true;
            hit.x = x; hit.y = y; hit.z = z;
            hit.distance = t;
            return hit;
        }
        // Step into whichever neighbouring cell the ray reaches first.
        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            x += stepX; t = tMaxX; tMaxX += deltaX;
            hit.nx = -stepX; hit.ny = 0; hit.nz = 0;
        } else if (tMaxY < tMaxZ) {
            y += stepY; t = tMaxY; tMaxY += deltaY;
            hit.nx = 0; hit.ny = -stepY; hit.nz = 0;
        } else {
            z += stepZ; t = tMaxZ; tMaxZ += deltaZ;
            hit.nx = 0; hit.ny = 0; hit.nz = -stepZ;
        }
    }
    return RayHit{};
}
