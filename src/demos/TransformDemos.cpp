// 12 2D transformations and 13 3D transformations (Checkpoint 2).
#include <cmath>
#include <cstdio>

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

const Color PANEL_BG{30, 33, 44};

// Block drawn at local cell (0,0,0) of `view`, transformed by m and faded towards the panel colour.
void drawBlockXf(Canvas& c, const IsoView& view, Block b, const Mat3& m, float fade = 0) {
    BlockFaces f = isoBlockFaces(view, 0, 0, 0);
    Color base = blockBaseColor(b), side = b == Block::Grass ? blockBaseColor(Block::Dirt) : base;
    struct { const Polygon* poly; Color col; } faces[] = {
        {&f.top, base}, {&f.left, shade(side, 0.8f)}, {&f.right, shade(side, 0.62f)}};
    for (auto& face : faces) {
        Polygon p = transformed(*face.poly, m);
        fillPolygonScanline(c, p, lerpColor(face.col, PANEL_BG, fade));
        drawPolygonOutline(c, p, lerpColor(shade(side, 0.45f), PANEL_BG, fade));
    }
}

std::string num(float v) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%5.2f", v);
    return buf;
}

void printMat3(Canvas& c, int x, int y, const Mat3& m, Color col) {
    for (int r = 0; r < 3; ++r)
        label(c, x, y + r * 11, "[" + num(m.m[r][0]) + " " + num(m.m[r][1]) + " " + num(m.m[r][2]) + "]", col, 1);
}

void printMat4(Canvas& c, int x, int y, const Mat4& m, Color col) {
    for (int r = 0; r < 4; ++r)
        label(c, x, y + r * 10,
              "[" + num(m.m[r][0]) + " " + num(m.m[r][1]) + " " + num(m.m[r][2]) + " " + num(m.m[r][3]) + "]", col, 1);
}

void panelAt(Canvas& c, int i, const std::string& title, int& px, int& py) {
    px = 10 + (i % 3) * 261;
    py = 76 + (i / 3) * 258;
    panel(c, px, py, 258, 252, title);
}

}  // namespace

Canvas transform2D() {
    Canvas c = makePage("12  2D TRANSFORMATIONS", "3X3 HOMOGENEOUS MATRICES APPLIED TO GAME COMPONENTS");
    int px, py;

    // 1. Translation: a hotbar block sliding into place.
    panelAt(c, 0, "TRANSLATION", px, py);
    IsoView v{float(px + 50), float(py + 180), 22};
    for (int k = 0; k <= 3; ++k)
        drawBlockXf(c, v, Block::Stone, Mat3::translate(k * 50.0f, -k * 30.0f), 0.75f - k * 0.25f);
    drawDashedLine(c, px + 50, py + 180, px + 200, py + 90, 4, 4, ACCENT);
    label(c, px + 8, py + 222, "T(50,-30) X3", TEXT);

    // 2. Rotation about a pivot: sun rays.
    panelAt(c, 1, "ROTATE ABOUT PIVOT", px, py);
    Vec2 sun{float(px + 129), float(py + 128)};
    Polygon ray = {{sun.x - 7, sun.y - 52}, {sun.x + 7, sun.y - 52}, {sun.x, sun.y - 82}};
    for (int k = 0; k < 12; ++k) {
        Polygon r = transformed(ray, Mat3::rotateAbout(sun, radians(30.0f * k)));
        fillPolygonScanline(c, r, k == 1 ? BAD : Color{255, 200, 80});
    }
    fillCircle(c, iround(sun.x), iround(sun.y), 40, {255, 214, 84});
    fillCircle(c, iround(sun.x), iround(sun.y), 3, {20, 20, 20});
    label(c, px + 8, py + 222, "R(30) ABOUT SUN X12", TEXT);

    // 3. Scaling about a fixed point (the block's bottom vertex stays on the ground).
    panelAt(c, 2, "SCALE ABOUT A POINT", px, py);
    const float scales[] = {0.5f, 1.0f, 1.5f};
    for (int k = 0; k < 3; ++k) {
        IsoView sv{float(px + 40 + k * 82), float(py + 160), 20};
        Vec2 foot = isoProject(sv, 1, 0, 1);
        drawBlockXf(c, sv, Block::Grass, Mat3::scaleAbout(foot, scales[k], scales[k]));
        fillCircle(c, iround(foot.x), iround(foot.y), 3, BAD);
    }
    c.fillRect(px + 8, py + 198, 242, 2, MUTED);
    label(c, px + 8, py + 222, "S(0.5)  S(1)  S(1.5)", TEXT);

    // 4. Reflection: a tree mirrored in the river.
    panelAt(c, 3, "REFLECTION", px, py);
    const float water = float(py + 150);
    c.fillRect(px + 1, iround(water), 256, 62, {58, 110, 190});
    Polygon trunk = {{float(px + 122), water}, {float(px + 134), water}, {float(px + 134), water - 40}, {float(px + 122), water - 40}};
    Polygon crown = regularPolygon({float(px + 128), water - 62}, 30, 10);
    Mat3 mirror = Mat3::reflectAboutHorizontal(water);
    const ClipRect lake{float(px + 1), water, float(px + 256), float(py + 211)};   // reflection stays in the water
    fillPolygonScanline(c, clipPolygonSutherlandHodgman(transformed(trunk, mirror), lake), {50, 70, 120});
    fillPolygonScanline(c, clipPolygonSutherlandHodgman(transformed(crown, mirror), lake), {44, 96, 130});
    fillPolygonScanline(c, trunk, blockBaseColor(Block::Wood));
    fillPolygonScanline(c, crown, blockBaseColor(Block::Leaves));
    drawDashedLine(c, px + 4, iround(water), px + 252, iround(water), 6, 4, TEXT);
    label(c, px + 8, py + 222, "MIRROR IN THE WATER", TEXT);

    // 5. Shear: the block's shadow on the ground.
    panelAt(c, 4, "SHEAR", px, py);
    const float ground = float(py + 170);
    Polygon box = {{float(px + 150), ground}, {float(px + 200), ground}, {float(px + 200), ground - 70}, {float(px + 150), ground - 70}};
    // Shear in x along the ground, then squash vertically, both about the ground line.
    Mat3 shadow = Mat3::translate(0, ground) * Mat3::scale(1, 0.35f) * Mat3::shear(1.4f, 0) * Mat3::translate(0, -ground);
    c.fillRect(px + 1, iround(ground), 256, 40, {74, 140, 70});
    fillPolygonScanline(c, transformed(box, shadow), {44, 84, 42});
    fillPolygonScanline(c, box, shade(blockBaseColor(Block::Stone), 0.85f));
    drawPolygonOutline(c, box, {60, 60, 66});
    fillCircle(c, px + 236, py + 50, 12, {255, 214, 84});
    label(c, px + 8, py + 222, "SHX=1.4, SY=0.35", TEXT);

    // 6. Composite: matrix order matters.
    panelAt(c, 5, "COMPOSITE ORDER", px, py);
    Vec2 o{float(px + 40), float(py + 170)};
    Polygon square = {{0, 0}, {36, 0}, {36, -36}, {0, -36}};
    Mat3 toPanel = Mat3::translate(o.x, o.y);
    Mat3 T = Mat3::translate(110, 0), R = Mat3::rotate(radians(-45));
    drawLineBresenham(c, iround(o.x), iround(o.y), px + 250, iround(o.y), MUTED);
    drawLineBresenham(c, iround(o.x), iround(o.y), iround(o.x), py + 32, MUTED);
    fillPolygonScanline(c, transformed(square, toPanel), {120, 120, 140});
    fillPolygonScanline(c, transformed(square, toPanel * T * R), GOOD);   // rotate, then move
    fillPolygonScanline(c, transformed(square, toPanel * R * T), BAD);    // move, then rotate about origin
    label(c, px + 160, py + 120, "T*R", GOOD);
    label(c, px + 180, py + 40, "R*T", BAD);
    printMat3(c, px + 8, py + 190, T * R, GOOD);
    return c;
}

Canvas transform3D() {
    Canvas c = makePage("13  3D TRANSFORMATIONS", "4X4 MATRICES: MODEL, VIEW (LOOK-AT), PERSPECTIVE");
    const DayNight light = dayNight(0.15f);
    const Vec3 centre{0.5f, 0.5f, 0.5f};
    const Mat4 toCentre = Mat4::translate(centre), fromCentre = Mat4::translate(centre * -1.0f);

    struct Case { const char* title; Mat4 model; bool ghost; };
    const Case cases[] = {
        {"MODEL = IDENTITY", Mat4::identity(), false},
        {"TRANSLATE (1.5,0,-1)", Mat4::translate({1.5f, 0, -1}), true},
        {"ROTATE Y 45", toCentre * Mat4::rotateY(radians(45)) * fromCentre, false},
        {"ROTATE X 30, Z 20", toCentre * Mat4::rotateX(radians(30)) * Mat4::rotateZ(radians(20)) * fromCentre, false},
        {"SCALE (1,2,1)", Mat4::scale({1, 2, 1}), false},
    };

    for (int i = 0; i < 6; ++i) {
        int px, py;
        panelAt(c, i, i < 5 ? cases[i].title : "VIEW + PERSPECTIVE", px, py);
        Canvas view(242, 150);
        drawSkyColors(view, 0, 150, {120, 170, 230}, {210, 230, 248});
        SoftRenderer r(view);
        r.setLight(light.sunDir, 0.45f);
        Mat4 proj = Mat4::perspective(radians(i < 5 ? 40.0f : 50.0f), 242.0f / 150.0f, 0.1f, 100.0f);
        if (i < 5) {
            Vec3 eye{3.6f, 3.0f, 5.0f};
            Mat4 cam = Mat4::lookAt(eye, {1.0f, 0.6f, 0.0f}, {0, 1, 0});
            r.setCamera(cam, proj, eye);
            for (int gx = -2; gx <= 3; ++gx)   // stone floor as a reference
                for (int gz = -3; gz <= 2; ++gz)
                    r.drawCube(Mat4::translate({float(gx), -1.0f, float(gz)}) * Mat4::scale({1, 0.999f, 1}), Block::Stone, 4);
            if (cases[i].ghost) r.drawCube(Mat4::identity(), Block::Sand);
            r.drawCube(cases[i].model, Block::Grass);
            printMat4(c, px + 8, py + 192, cases[i].model, TEXT);
        } else {
            VoxelWorld world = makeWorld();
            Vec3 eye{26, 17, 27};
            Mat4 cam = Mat4::lookAt(eye, {7.5f, 3, 7.5f}, {0, 1, 0});
            r.setCamera(cam, proj, eye);
            r.setFog({210, 230, 248}, 30, 60);
            int faces = r.drawWorld(world);
            label(c, px + 8, py + 192, "EYE (26,17,27)", TEXT);
            label(c, px + 8, py + 212, std::to_string(faces) + " VISIBLE FACES", MUTED);
        }
        c.blit(view, px + 8, py + 32);
    }
    return c;
}

}  // namespace demo
