// Checkpoint 2 renderer: transformations and animation, pure C++.
// Writes the transformation pages, one contact sheet per animation and every
// animation frame:
//   outputs/cp2/12_transform_2d.ppm ... 17_anim_walk.ppm
//   outputs/cp2/frames/<animation>/frame_000.ppm ...
//
// Usage: animator [output-directory]     (default: outputs/cp2)
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>

#include "demos/Demos.h"

int main(int argc, char** argv) {
    const std::string outDir = argc > 1 ? argv[1] : "outputs/cp2";
    std::filesystem::create_directories(outDir);
    int failed = 0;
    auto save = [&](const Canvas& c, const std::string& path) {
        if (c.savePPM(path)) std::cout << "saved  " << path << "\n";
        else { std::cerr << "FAILED " << path << "\n"; ++failed; }
    };

    save(demo::transform2D(), outDir + "/12_transform_2d.ppm");
    save(demo::transform3D(), outDir + "/13_transform_3d.ppm");

    int number = 14;
    for (const demo::Animation& anim : demo::animations()) {
        save(demo::contactSheet(anim), outDir + "/" + std::to_string(number++) + "_anim_" + anim.name + ".ppm");
        const std::string frameDir = outDir + "/frames/" + anim.name;
        std::filesystem::create_directories(frameDir);
        for (int f = 0; f < anim.frames; ++f) {
            Canvas frame(anim.width, anim.height);
            anim.render(frame, f);
            char name[32];
            std::snprintf(name, sizeof name, "/frame_%03d.ppm", f);
            if (!frame.savePPM(frameDir + name)) ++failed;
        }
        std::cout << "saved  " << frameDir << "/ (" << anim.frames << " frames)\n";
    }
    return failed ? 1 : 0;
}
