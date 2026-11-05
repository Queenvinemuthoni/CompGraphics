#include "engine/WorldMesh.h"

#include <vector>

Color blockFaceColor(Block b, int face) {
    if (b == Block::Grass && face != 2) return blockBaseColor(Block::Dirt);
    return blockBaseColor(b);
}

void WorldMesh::rebuild(const VoxelWorld& world, const std::function<bool(int, int, int)>& skip) {
    static const int dir[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    std::vector<Vertex> solid, water;
    for (int y = 0; y < world.sizeY(); ++y)
        for (int z = 0; z < world.sizeZ(); ++z)
            for (int x = 0; x < world.sizeX(); ++x) {
                Block b = world.get(x, y, z);
                if (b == Block::Air || skip(x, y, z)) continue;
                for (int f = 0; f < 6; ++f) {
                    int nx = x + dir[f][0], ny = y + dir[f][1], nz = z + dir[f][2];
                    Block n = world.get(nx, ny, nz);
                    // A neighbour that is animating is not drawn by this mesh, so treat it as open.
                    bool open = !isOpaque(n) || skip(nx, ny, nz);
                    if (b == Block::Water) {
                        if (n == Block::Air || skip(nx, ny, nz)) {
                            bool surface = world.get(x, y + 1, z) == Block::Air;
                            appendCubeFace(water, float(x), float(y), float(z), f, blockFaceColor(b, f), surface ? 0.88f : 1.0f);
                        }
                    } else if (open) {
                        appendCubeFace(solid, float(x), float(y), float(z), f, blockFaceColor(b, f));
                    }
                }
            }
    opaque_.upload(solid);
    water_.upload(water);
}
