#pragma once
// Voxel World Builder components built from the CP1 raster algorithms.
// Each block is drawn in isometric view as three filled polygons
// (top, left and right faces) shaded with HSV colour theory.
#include <string>
#include <vector>

#include "core/Canvas.h"
#include "core/Geometry.h"
#include "math/Transform.h"

enum class Block { Air, Grass, Dirt, Stone, Sand, Water, Wood, Leaves };

Color blockBaseColor(Block b);
const char* blockName(Block b);
bool isOpaque(Block b);

// Isometric projection: x and z run along the ground, y points up.
struct IsoView {
    float originX, originY;  // screen position of world point (0,0,0)
    float tile;              // half the screen width of one block
};

Vec2 isoProject(const IsoView& view, float x, float y, float z);

struct BlockFaces {
    Polygon top, left, right;
};
BlockFaces isoBlockFaces(const IsoView& view, int x, int y, int z);
// Outer silhouette of a block (hexagon), used for selection highlights.
Polygon isoBlockSilhouette(const IsoView& view, int x, int y, int z);

struct FaceMask {
    bool top = true, left = true, right = true;
};

// Draws one block. Optionally every face polygon is first transformed by a 2D
// matrix (used by the animations) and/or clipped to a window (Sutherland-Hodgman).
void drawIsoBlock(Canvas& canvas, const IsoView& view, int x, int y, int z, Block b,
                  FaceMask faces = {}, const ClipRect* clip = nullptr, bool outline = true,
                  const Mat3* transform = nullptr);

class VoxelWorld {
public:
    VoxelWorld(int sizeX, int sizeY, int sizeZ);
    int sizeX() const { return sx_; }
    int sizeY() const { return sy_; }
    int sizeZ() const { return sz_; }
    Block get(int x, int y, int z) const;   // Air outside the world
    void set(int x, int y, int z, Block b);
    int surfaceHeight(int x, int z) const;  // index of the highest non-air block, -1 if none
    void clear();

    // Binary file: "VOXW", three int32 sizes, then one byte per block.
    bool save(const std::string& path) const;
    bool load(const std::string& path);

    // Deterministic terrain: rolling hills, a winding river with sandy banks, trees.
    void generateTerrain();

private:
    int sx_, sy_, sz_;
    std::vector<Block> blocks_;
};

struct WorldDrawStats {
    int blocks = 0;
    int facesDrawn = 0;
    int facesCulled = 0;
};

// Painter's algorithm (back to front by x+y+z) with hidden-face culling:
// a face is skipped when the neighbouring block covers it.
WorldDrawStats drawWorldIso(Canvas& canvas, const IsoView& view, const VoxelWorld& world,
                            const ClipRect* clip = nullptr, bool outline = true);
