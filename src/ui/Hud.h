#pragma once
// Game HUD drawn with the CP1 raster algorithms. Used by the pure C++ renders
// and, through a texture, by the OpenGL engine.
#include <string>

#include "core/Canvas.h"
#include "scene/Voxel.h"

constexpr int HOTBAR_SIZE = 7;
extern const Block HOTBAR_BLOCKS[HOTBAR_SIZE];

// Circle + four line arms + centre dot.
void drawCrosshair(Canvas& c, int x, int y, Color col = {255, 255, 255});
// Dashed outline of a block in isometric view.
void drawSelection(Canvas& c, const IsoView& view, int x, int y, int z, Color col);
// Isometric block icon on a colour-key background (for BitBlt with transparency).
Canvas makeBlockIcon(Block b, int size, Color key);
// Seven slots with icons; the selected one is framed and named above the bar.
// If counts is given (one per slot), each slot shows how many blocks the player
// holds and empty slots are dimmed.
void drawHotbar(Canvas& c, int centerX, int y, int selected, const int* counts = nullptr);
// Top-down map. (px, pz) is the player in block units; heading is the view
// direction in map space (radians, 0 = +x, PI/2 = +z).
void drawMinimap(Canvas& c, const VoxelWorld& world, int mx, int my, int cell, float px, float pz,
                 float heading, bool riverOverlay = true);
// Text with a 1-pixel drop shadow so it stays readable on any background.
void drawShadowText(Canvas& c, int x, int y, const std::string& text, Color col, int scale = 2);
