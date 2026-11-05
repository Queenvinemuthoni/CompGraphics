// Checkpoint 2 animations, rendered frame by frame:
//   day_night   - keyframed sky colours, sun/moon rotating about a pivot, drifting clouds
//   build       - blocks dropping in with bounce easing, one removed by scaling
//   flythrough  - 3D camera moving along a closed Catmull-Rom spline (software renderer)
//   walk        - builder walk cycle (limbs rotated about joints) with parallax scrolling
#include <cmath>
#include <cstdint>

#include "anim/Animation.h"
#include "color/ColorOps.h"
#include "core/Font.h"
#include "demos/Demos.h"
#include "math/Transform.h"
#include "raster/Circles.h"
#include "raster/Lines.h"
#include "raster/Polygons.h"
#include "scene/Renderer3D.h"

namespace demo {

namespace {

const Color KEY{255, 0, 255};

// Multiplies every non-key pixel by a light colour (used to darken the scene at night).
void modulate(Canvas& c, Color tint) {
    for (int y = 0; y < c.height(); ++y)
        for (int x = 0; x < c.width(); ++x) {
            Color p = c.getPixel(x, y);
            if (p == KEY) continue;
            c.setPixel(x, y, rgb(p.r * tint.r / 255, p.g * tint.g / 255, p.b * tint.b / 255));
        }
}

// ---------------------------------------------------------------- day / night
void renderDayNight(Canvas& c, int frame, int frames) {
    const float t = float(frame) / frames;
    const DayNight dn = dayNight(t);
    drawSkyColors(c, 0, c.height(), dn.skyTop, dn.skyBottom);

    // Stars fade in at night.
    uint32_t seed = 7;
    for (int i = 0; i < 70; ++i) {
        seed = seed * 1664525u + 1013904223u;
        int sx = int(seed % 480);
        seed = seed * 1664525u + 1013904223u;
        int sy = int(seed % 200);
        Color sky = c.getPixel(sx, sy);
        c.setPixel(sx, sy, lerpColor(sky, {255, 255, 255}, dn.starAlpha));
        if (i % 7 == 0) c.setPixel(sx + 1, sy, lerpColor(sky, {255, 255, 255}, dn.starAlpha * 0.6f));
    }

    // Sun and moon orbit a pivot below the horizon: rotation about a fixed point.
    const Vec2 pivot{240, 330};
    const Vec2 start{240 - 220, 330};   // sunrise in the east (left)
    Vec2 sun = Mat3::rotateAbout(pivot, dn.sunAngle).apply(start);
    Vec2 moon = Mat3::rotateAbout(pivot, dn.sunAngle + PI).apply(start);
    fillCircle(c, iround(moon.x), iround(moon.y), 15, {236, 236, 220});
    fillCircle(c, iround(moon.x) + 7, iround(moon.y) - 4, 13, lerpColor(dn.skyTop, dn.skyBottom, moon.y / 360.0f));
    drawCircle(c, iround(sun.x), iround(sun.y), 30, {255, 236, 170});
    fillCircle(c, iround(sun.x), iround(sun.y), 22, {255, 214, 84});

    // Everything that is lit goes on a layer which is then tinted by the light colour.
    Canvas layer(c.width(), c.height(), KEY);
    for (int k = 0; k < 3; ++k) {
        float x = std::fmod(60.0f + k * 190 + frame * (1.5f + k * 0.5f), 620.0f) - 70;   // translation with wrap
        drawCloud(layer, iround(x), 50 + k * 34, 16 + k * 3);
    }
    drawHills(layer, 250);
    static const VoxelWorld world = makeWorld();
    drawWorldIso(layer, IsoView{240, 172, 9}, world);
    float bright = 0.35f + 0.65f * dn.daylight;
    modulate(layer, rgb(int(dn.lightTint.r * bright), int(dn.lightTint.g * bright), int(dn.lightTint.b * bright)));
    c.blit(layer, 0, 0, &KEY);

    drawShadowText(c, 12, 12, clockText(t), {255, 255, 255}, 3);
}

// ---------------------------------------------------------------- building
struct Placement {
    int x, y, z;
    Block b;
};

std::vector<Placement> housePlan() {
    std::vector<Placement> plan;
    for (int y = 1; y <= 2; ++y)
        for (int x = 1; x <= 3; ++x)
            for (int z = 1; z <= 3; ++z) {
                bool wall = x == 1 || x == 3 || z == 1 || z == 3;
                bool door = x == 2 && z == 3;
                if (wall && !door) plan.push_back({x, y, z, Block::Stone});
            }
    for (int x = 1; x <= 3; ++x)
        for (int z = 1; z <= 3; ++z) plan.push_back({x, 3, z, Block::Wood});
    plan.push_back({2, 4, 2, Block::Leaves});
    return plan;
}

void renderBuild(Canvas& c, int frame, int /*frames*/) {
    drawSky(c, 0, c.height());
    const IsoView view{240, 150, 26};
    const std::vector<Placement> plan = housePlan();
    const int stagger = 2, dropFrames = 10;
    const int removeStart = int(plan.size()) * stagger + dropFrames + 6, removeFrames = 10;

    VoxelWorld world(6, 6, 6);
    for (int x = 0; x < 6; ++x)
        for (int z = 0; z < 6; ++z) world.set(x, 0, z, Block::Grass);

    struct Flying { Placement p; float t; };
    std::vector<Flying> flying;
    for (size_t i = 0; i < plan.size(); ++i) {
        float p = progress(float(frame), float(i * stagger), float(dropFrames));
        if (p >= 1) world.set(plan[i].x, plan[i].y, plan[i].z, plan[i].b);
        else if (p > 0) flying.push_back({plan[i], p});
    }
    // The roof-top leaves block is removed at the end by shrinking it.
    const Placement& last = plan.back();
    float shrink = progress(float(frame), float(removeStart), float(removeFrames));
    if (shrink > 0) world.set(last.x, last.y, last.z, Block::Air);

    drawWorldIso(c, view, world);
    for (const Flying& f : flying) {
        // Shadow grows as the block approaches its cell.
        Vec2 spot = isoProject(view, f.p.x + 0.5f, float(f.p.y), f.p.z + 0.5f);
        fillEllipse(c, iround(spot.x), iround(spot.y), iround(6 + 14 * f.t), iround(3 + 7 * f.t), {40, 60, 40});
        float lift = (1 - ease::outBounce(f.t)) * 220;   // translation driven by bounce easing
        Mat3 drop = Mat3::translate(0, -lift);
        drawIsoBlock(c, view, f.p.x, f.p.y, f.p.z, f.p.b, {}, nullptr, true, &drop);
    }
    if (shrink > 0 && shrink < 1) {
        Vec2 centre = isoProject(view, last.x + 0.5f, last.y + 0.5f, last.z + 0.5f);
        float s = 1 - ease::inQuad(shrink);
        Mat3 m = Mat3::rotateAbout(centre, shrink * 1.5f) * Mat3::scaleAbout(centre, s, s);
        drawIsoBlock(c, view, last.x, last.y, last.z, last.b, {}, nullptr, true, &m);
    }
    const char* caption = frame < removeStart ? "PLACE: TRANSLATE + BOUNCE EASING" : "REMOVE: SCALE + ROTATE ABOUT CENTRE";
    drawShadowText(c, 12, 12, caption, {255, 255, 255});
}

// ---------------------------------------------------------------- fly-through
Vec2 catmullRomClosed(const std::vector<Vec2>& p, float u) {
    int n = int(p.size());
    float s = u * n;
    int i = int(std::floor(s));
    float t = s - i;
    auto at = [&](int k) { return p[((k % n) + n) % n]; };
    Vec2 p0 = at(i - 1), p1 = at(i), p2 = at(i + 1), p3 = at(i + 2);
    float t2 = t * t, t3 = t2 * t;
    return (p1 * 2.0f + (p2 - p0) * t + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
            (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;
}

void renderFlythrough(Canvas& c, int frame, int frames) {
    static const VoxelWorld world = makeWorld();
    const DayNight dn = dayNight(0.12f);
    drawSkyColors(c, 0, c.height(), dn.skyTop, dn.skyBottom);

    // Camera key positions around the island (x, z); a closed Catmull-Rom spline joins them.
    static const std::vector<Vec2> keys = {{-13, 6}, {-3, -11}, {14, -13}, {28, -1}, {27, 17}, {12, 28}, {-7, 23}};
    float u = float(frame) / frames;
    Vec2 xz = catmullRomClosed(keys, u);
    Vec3 eye{xz.x, 12.0f + 4.0f * std::sin(u * 2 * PI), xz.y};
    Vec3 target{7.5f, 3.0f, 7.5f};

    SoftRenderer r(c);
    r.setCamera(Mat4::lookAt(eye, target, {0, 1, 0}),
                Mat4::perspective(radians(60), float(c.width()) / c.height(), 0.1f, 100.0f), eye);
    r.setLight(dn.sunDir, dn.ambient, dn.lightTint);
    r.setFog(dn.skyBottom, 18, 40);
    r.drawWorld(world);
    drawShadowText(c, 12, 12, "CAMERA ON A CLOSED SPLINE", {255, 255, 255});
}

// ---------------------------------------------------------------- walk cycle
Polygon limb(Vec2 joint, float w, float len) {
    return {{joint.x - w / 2, joint.y}, {joint.x + w / 2, joint.y}, {joint.x + w / 2, joint.y + len}, {joint.x - w / 2, joint.y + len}};
}

// s scales every dimension of the character.
void drawWalker(Canvas& c, Vec2 hip, float phase, float s) {
    float swing = 0.55f * std::sin(phase);
    float bob = -3.0f * s * std::fabs(std::cos(phase));   // body rises at mid-stride
    hip = hip + Vec2{0, bob};
    Vec2 shoulder = hip + Vec2{0, -34 * s};
    const Color pants{60, 60, 80}, shirt{60, 110, 200}, skin{240, 196, 150};
    // Back limbs first (darker), then body, then front limbs: limbs rotate about hip/shoulder joints.
    fillPolygonScanline(c, transformed(limb(shoulder, 8 * s, 28 * s), Mat3::rotateAbout(shoulder, swing)), shade(skin, 0.7f));
    fillPolygonScanline(c, transformed(limb(hip, 10 * s, 34 * s), Mat3::rotateAbout(hip, -swing)), shade(pants, 0.7f));
    fillEllipse(c, iround(hip.x), iround(hip.y - 18 * s), iround(15 * s), iround(22 * s), shirt);
    drawEllipse(c, iround(hip.x), iround(hip.y - 18 * s), iround(15 * s), iround(22 * s), {30, 56, 110});
    fillPolygonScanline(c, transformed(limb(hip, 10 * s, 34 * s), Mat3::rotateAbout(hip, swing)), pants);
    fillPolygonScanline(c, transformed(limb(shoulder, 8 * s, 28 * s), Mat3::rotateAbout(shoulder, -swing)), skin);
    Vec2 head = shoulder + Vec2{0, -16 * s};
    fillCircle(c, iround(head.x), iround(head.y), iround(13 * s), skin);
    drawCircle(c, iround(head.x), iround(head.y), iround(13 * s), {150, 110, 80});
    fillEllipse(c, iround(head.x), iround(head.y - 10 * s), iround(14 * s), iround(6 * s), {255, 200, 40});
    fillCircle(c, iround(head.x + 6 * s), iround(head.y), iround(2 * s), {30, 30, 40});
}

void renderWalk(Canvas& c, int frame, int /*frames*/) {
    drawSky(c, 0, c.height());
    const float scroll = frame * 5.0f;   // camera follows the walker: the world moves left
    for (int k = 0; k < 3; ++k) {
        float x = std::fmod(600.0f + k * 210 - frame * 1.0f, 640.0f) - 80;
        drawCloud(c, iround(x), 50 + k * 30, 14 + k * 3);
    }
    drawHills(c, 230, scroll * 0.5f);   // background moves slower: parallax
    const int ground = 280, size = 40;
    for (int i = -1; i <= c.width() / size + 1; ++i) {
        int x = i * size - int(std::fmod(scroll, float(size)));
        c.fillRect(x, ground, size, c.height() - ground, blockBaseColor(Block::Dirt));
        c.fillRect(x, ground, size, 9, blockBaseColor(Block::Grass));
        drawRectOutline(c, {float(x), float(ground), float(x + size - 1), float(c.height() - 1)}, shade(blockBaseColor(Block::Dirt), 0.6f));
    }
    float phase = (frame + 4) * 2 * PI / 16;   // one stride every 16 frames
    const float s = 1.8f;
    fillEllipse(c, 200, ground, 36, 7, shade(blockBaseColor(Block::Grass), 0.6f));
    drawWalker(c, {200, ground - 34 * s}, phase, s);
    drawShadowText(c, 12, 12, "LIMBS ROTATE ABOUT JOINTS", {255, 255, 255});
}

}  // namespace

std::vector<Animation> animations() {
    return {
        {"day_night", "14  ANIMATION: DAY AND NIGHT", "KEYFRAMED SKY COLOUR, SUN ROTATING ABOUT A PIVOT, CLOUDS",
         96, 480, 360, [](Canvas& c, int f) { renderDayNight(c, f, 96); }},
        {"build", "15  ANIMATION: PLACE AND REMOVE", "TRANSLATION WITH BOUNCE EASING, SCALE + ROTATE TO REMOVE",
         84, 480, 360, [](Canvas& c, int f) { renderBuild(c, f, 84); }},
        {"flythrough", "16  ANIMATION: CAMERA FLY-THROUGH", "LOOK-AT CAMERA ON A SPLINE, PERSPECTIVE, Z-BUFFER, FOG",
         96, 480, 360, [](Canvas& c, int f) { renderFlythrough(c, f, 96); }},
        {"walk", "17  ANIMATION: WALK CYCLE", "ROTATION ABOUT JOINTS, BODY BOB, PARALLAX SCROLLING",
         64, 480, 360, [](Canvas& c, int f) { renderWalk(c, f, 64); }},
    };
}

Canvas contactSheet(const Animation& anim) {
    Canvas c = makePage(anim.title, anim.subtitle);
    for (int i = 0; i < 8; ++i) {
        int frame = i * anim.frames / 8;
        Canvas f(anim.width, anim.height);
        anim.render(f, frame);
        int x = 10 + (i % 4) * 197, y = 80 + (i / 4) * 256;
        c.blitScaled(f, x, y, 190, 142);
        drawRectOutline(c, {float(x - 1), float(y - 1), float(x + 190), float(y + 142)}, {58, 62, 80});
        label(c, x, y + 150, "FRAME " + std::to_string(frame), MUTED);
    }
    label(c, 16, 576, std::to_string(anim.frames) + " FRAMES - SEE OUTPUTS/CP2/GIF/" + anim.name + ".GIF", TEXT);
    return c;
}

}  // namespace demo
