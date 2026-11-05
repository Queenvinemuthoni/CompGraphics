// Checkpoint 1 renderer: pure C++, no graphics library.
// Renders every technique demo to outputs/cp1/*.ppm.
//
// Usage: renderer [output-directory]     (default: outputs/cp1)
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>

#include "demos/Demos.h"

int main(int argc, char** argv) {
    const std::string outDir = argc > 1 ? argv[1] : "outputs/cp1";
    std::filesystem::create_directories(outDir);

    struct Job { const char* file; std::function<Canvas()> render; };
    const Job jobs[] = {
        {"01_lines.ppm", demo::lines},
        {"02_line_clipping.ppm", demo::lineClipping},
        {"03_circles_ellipses.ppm", demo::circlesEllipses},
        {"04_polygon_fill.ppm", demo::polygonFill},
        {"05_polygon_clipping.ppm", demo::polygonClipping},
        {"06_curves.ppm", demo::curves},
        {"07_colour_theory.ppm", demo::colorTheory},
        {"08_colour_reduction.ppm", demo::colorReduction},
        {"09_raster_ops.ppm", demo::rasterOps},
        {"10_visual_concept.ppm", demo::visualConcept},
        {"11_preview_3d.ppm", demo::preview3D},
    };

    int failed = 0;
    for (const Job& job : jobs) {
        const std::string path = outDir + "/" + job.file;
        if (job.render().savePPM(path)) std::cout << "saved  " << path << "\n";
        else { std::cerr << "FAILED " << path << "\n"; ++failed; }
    }
    return failed ? 1 : 0;
}
