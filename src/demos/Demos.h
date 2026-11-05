#pragma once
// One function per output image. Each demo shows a technique applied to a
// component of the Voxel World Builder.
#include <functional>
#include <string>
#include <vector>

#include "core/Canvas.h"
#include "core/Geometry.h"
#include "scene/Voxel.h"
#include "ui/Hud.h"

namespace demo {

constexpr int W = 800, H = 600;

// Shared look for every output image.
extern const Color BG, PANEL, TEXT, MUTED, ACCENT, GOOD, BAD;

Canvas makePage(const std::string& title, const std::string& subtitle);
void label(Canvas& c, int x, int y, const std::string& text, Color col = MUTED, int scale = 2);
void panel(Canvas& c, int x, int y, int w, int h, const std::string& title);

// Scene pieces reused across demos.
VoxelWorld makeWorld();
void drawSky(Canvas& c, int top, int bottom);
void drawSkyColors(Canvas& c, int top, int bottom, Color skyTop, Color skyBottom);
void drawCloud(Canvas& c, int x, int y, int size);
void drawSun(Canvas& c, int x, int y, int r);
void drawHills(Canvas& c, int baseY, float scrollX = 0, Color tint = {255, 255, 255});
Canvas renderConcept();   // the full visual concept scene, 800x600

// ---- Checkpoint 1 ----
Canvas lines();             // 01
Canvas lineClipping();      // 02
Canvas circlesEllipses();   // 03
Canvas polygonFill();       // 04
Canvas polygonClipping();   // 05
Canvas curves();            // 06
Canvas colorTheory();       // 07
Canvas colorReduction();    // 08
Canvas rasterOps();         // 09
Canvas visualConcept();     // 10
Canvas preview3D();         // 11

// ---- Checkpoint 2 ----
Canvas transform2D();       // 12
Canvas transform3D();       // 13

// A frame-by-frame animation: render(canvas, frameIndex) draws one frame.
struct Animation {
    std::string name;
    std::string title;
    std::string subtitle;
    int frames, width, height;
    std::function<void(Canvas&, int)> render;
};
std::vector<Animation> animations();
// A page showing 8 evenly spaced frames of an animation.
Canvas contactSheet(const Animation& anim);

}  // namespace demo
