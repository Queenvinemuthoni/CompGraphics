#include "scene/Voxel.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <tuple>

#include "color/ColorOps.h"
#include "raster/Polygons.h"

Color blockBaseColor(Block b) {
    switch (b) {
        case Block::Grass:  return {92, 184, 72};
        case Block::Dirt:   return {134, 96, 60};
        case Block::Stone:  return {128, 128, 134};
        case Block::Sand:   return {222, 206, 146};
        case Block::Water:  return {58, 120, 216};
        case Block::Wood:   return {92, 60, 34};
        case Block::Leaves: return {46, 132, 58};
        default:            return {0, 0, 0};
    }
}

const char* blockName(Block b) {
    switch (b) {
        case Block::Grass:  return "GRASS";
        case Block::Dirt:   return "DIRT";
        case Block::Stone:  return "STONE";
        case Block::Sand:   return "SAND";
        case Block::Water:  return "WATER";
        case Block::Wood:   return "WOOD";
        case Block::Leaves: return "LEAVES";
        default:            return "AIR";
    }
}

bool isOpaque(Block b) { return b != Block::Air && b != Block::Water; }

Vec2 isoProject(const IsoView& view, float x, float y, float z) {
    return {view.originX + (x - z) * view.tile,
            view.originY + (x + z) * view.tile * 0.5f - y * view.tile};
}

BlockFaces isoBlockFaces(const IsoView& v, int x, int y, int z) {
    auto P = [&](float a, float b, float c) { return isoProject(v, a, b, c); };
    BlockFaces f;
    f.top = {P(x, y + 1, z), P(x + 1, y + 1, z), P(x + 1, y + 1, z + 1), P(x, y + 1, z + 1)};
    f.right = {P(x + 1, y + 1, z), P(x + 1, y + 1, z + 1), P(x + 1, y, z + 1), P(x + 1, y, z)};
    f.left = {P(x, y + 1, z + 1), P(x + 1, y + 1, z + 1), P(x + 1, y, z + 1), P(x, y, z + 1)};
    return f;
}

Polygon isoBlockSilhouette(const IsoView& v, int x, int y, int z) {
    auto P = [&](float a, float b, float c) { return isoProject(v, a, b, c); };
    return {P(x, y + 1, z), P(x + 1, y + 1, z), P(x + 1, y, z), P(x + 1, y, z + 1),
            P(x, y, z + 1), P(x, y + 1, z + 1)};
}

namespace {

void fillFace(Canvas& canvas, const Polygon& face, Color fill, Color edge,
              const ClipRect* clip, bool outline, const Mat3* xf) {
    Polygon p = xf ? transformed(face, *xf) : face;
    if (clip) p = clipPolygonSutherlandHodgman(p, *clip);
    if (p.size() < 3) return;
    fillPolygonScanline(canvas, p, fill);
    if (outline) drawPolygonOutline(canvas, p, edge);
}

}  // namespace

void drawIsoBlock(Canvas& canvas, const IsoView& view, int x, int y, int z, Block b,
                  FaceMask faces, const ClipRect* clip, bool outline, const Mat3* xf) {
    if (b == Block::Air) return;
    BlockFaces f = isoBlockFaces(view, x, y, z);
    Color base = blockBaseColor(b);
    // Fixed light from the upper-left: top is brightest, right side darkest.
    Color top = base, left = shade(base, 0.80f), right = shade(base, 0.62f);
    Color edge = shade(base, 0.45f);
    if (b == Block::Grass) {   // grass blocks show dirt on the sides with a grass band
        Color dirt = blockBaseColor(Block::Dirt);
        left = shade(dirt, 0.80f);
        right = shade(dirt, 0.62f);
        edge = shade(dirt, 0.45f);
    }

    if (faces.top) fillFace(canvas, f.top, top, edge, clip, outline, xf);
    if (faces.left) fillFace(canvas, f.left, left, edge, clip, outline, xf);
    if (faces.right) fillFace(canvas, f.right, right, edge, clip, outline, xf);

    if (b == Block::Grass) {
        auto P = [&](float a, float bb, float c) { return isoProject(view, a, bb, c); };
        const float band = y + 0.78f;
        if (faces.left)
            fillFace(canvas, {P(x, y + 1, z + 1), P(x + 1, y + 1, z + 1), P(x + 1, band, z + 1), P(x, band, z + 1)},
                     shade(base, 0.80f), shade(base, 0.5f), clip, false, xf);
        if (faces.right)
            fillFace(canvas, {P(x + 1, y + 1, z), P(x + 1, y + 1, z + 1), P(x + 1, band, z + 1), P(x + 1, band, z)},
                     shade(base, 0.62f), shade(base, 0.5f), clip, false, xf);
    }
}

VoxelWorld::VoxelWorld(int sizeX, int sizeY, int sizeZ)
    : sx_(sizeX), sy_(sizeY), sz_(sizeZ),
      blocks_(static_cast<size_t>(sizeX) * sizeY * sizeZ, Block::Air) {}

Block VoxelWorld::get(int x, int y, int z) const {
    if (x < 0 || y < 0 || z < 0 || x >= sx_ || y >= sy_ || z >= sz_) return Block::Air;
    return blocks_[(static_cast<size_t>(y) * sz_ + z) * sx_ + x];
}

void VoxelWorld::set(int x, int y, int z, Block b) {
    if (x < 0 || y < 0 || z < 0 || x >= sx_ || y >= sy_ || z >= sz_) return;
    blocks_[(static_cast<size_t>(y) * sz_ + z) * sx_ + x] = b;
}

void VoxelWorld::clear() { std::fill(blocks_.begin(), blocks_.end(), Block::Air); }

bool VoxelWorld::save(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    int32_t dims[3] = {sx_, sy_, sz_};
    out.write("VOXW", 4);
    out.write(reinterpret_cast<const char*>(dims), sizeof dims);
    for (Block b : blocks_) out.put(static_cast<char>(b));
    return static_cast<bool>(out);
}

bool VoxelWorld::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    char magic[4];
    int32_t dims[3];
    if (!in.read(magic, 4) || std::string(magic, 4) != "VOXW") return false;
    if (!in.read(reinterpret_cast<char*>(dims), sizeof dims)) return false;
    if (dims[0] != sx_ || dims[1] != sy_ || dims[2] != sz_) return false;
    std::vector<Block> data(blocks_.size());
    for (Block& b : data) {
        int v = in.get();
        if (v < 0 || v > static_cast<int>(Block::Leaves)) return false;
        b = static_cast<Block>(v);
    }
    blocks_ = data;
    return true;
}

int VoxelWorld::surfaceHeight(int x, int z) const {
    for (int y = sy_ - 1; y >= 0; --y)
        if (get(x, y, z) != Block::Air) return y;
    return -1;
}

void VoxelWorld::generateTerrain() {
    const int waterLevel = 2;
    for (int x = 0; x < sx_; ++x)
        for (int z = 0; z < sz_; ++z) {
            float n = 1.6f * std::sin(x * 0.55f) + 1.3f * std::cos(z * 0.45f + 0.8f) +
                      0.8f * std::sin((x + z) * 0.3f);
            int h = std::clamp(4 + static_cast<int>(std::lround(n)), 3, 7);
            // The river winds across the map along z = 7 + 2.5 sin(0.5 x).
            float riverDist = std::fabs(z - (7.0f + 2.5f * std::sin(x * 0.5f)));
            bool river = riverDist < 1.2f, bank = riverDist < 2.4f;
            if (river) h = waterLevel - 1;
            else if (bank) h = waterLevel;

            for (int y = 0; y <= h; ++y) {
                Block b = Block::Stone;
                if (y == h) b = bank ? Block::Sand : Block::Grass;
                else if (y >= h - 2) b = bank || river ? Block::Sand : Block::Dirt;
                set(x, y, z, b);
            }
            if (river) set(x, waterLevel, z, Block::Water);
        }

    // A few trees on high ground.
    const int trees[][2] = {{2, 2}, {11, 3}, {3, 12}, {12, 12}};
    for (auto& t : trees) {
        int x = t[0], z = t[1];
        int ground = surfaceHeight(x, z);
        if (get(x, ground, z) != Block::Grass) continue;
        for (int y = ground + 1; y <= ground + 3; ++y) set(x, y, z, Block::Wood);
        for (int dx = -1; dx <= 1; ++dx)
            for (int dz = -1; dz <= 1; ++dz)
                for (int dy = 3; dy <= 4; ++dy)
                    if ((dx || dz || dy == 4) && get(x + dx, ground + dy, z + dz) == Block::Air)
                        set(x + dx, ground + dy, z + dz, Block::Leaves);
        set(x, ground + 5, z, Block::Leaves);
    }
}

WorldDrawStats drawWorldIso(Canvas& canvas, const IsoView& view, const VoxelWorld& world,
                            const ClipRect* clip, bool outline) {
    std::vector<std::tuple<int, int, int, int>> order;  // (depth, x, y, z)
    for (int y = 0; y < world.sizeY(); ++y)
        for (int z = 0; z < world.sizeZ(); ++z)
            for (int x = 0; x < world.sizeX(); ++x)
                if (world.get(x, y, z) != Block::Air) order.emplace_back(x + y + z, x, y, z);
    std::sort(order.begin(), order.end());

    WorldDrawStats stats;
    for (auto& [depth, x, y, z] : order) {
        Block b = world.get(x, y, z);
        FaceMask m;
        if (b == Block::Water) {   // only the water surface is visible
            m = {world.get(x, y + 1, z) == Block::Air, false, false};
        } else {
            m.top = !isOpaque(world.get(x, y + 1, z));
            m.left = !isOpaque(world.get(x, y, z + 1));
            m.right = !isOpaque(world.get(x + 1, y, z));
        }
        int visible = m.top + m.left + m.right;
        ++stats.blocks;
        stats.facesDrawn += visible;
        stats.facesCulled += 6 - visible;   // 3 back faces are never visible in this view
        if (visible) drawIsoBlock(canvas, view, x, y, z, b, m, clip, outline && b != Block::Water);
    }
    return stats;
}
