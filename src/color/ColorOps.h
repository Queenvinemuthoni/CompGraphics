#pragma once
// Colour theory and colour space reduction.
//   - RGB <-> HSV conversion (used to derive light/shadow shades of each block)
//   - Luminance-based greyscale
//   - Palettes: uniform (e.g. 3-3-2 bits) and popularity (most frequent colours)
//   - Reduction: nearest-colour quantisation, ordered (Bayer) dithering and
//     Floyd-Steinberg error-diffusion dithering
#include <vector>

#include "core/Canvas.h"

struct HSV {
    float h;  // hue in degrees [0, 360)
    float s;  // saturation [0, 1]
    float v;  // value [0, 1]
};

HSV rgbToHsv(Color c);
Color hsvToRgb(HSV hsv);
Color shade(Color c, float valueFactor);  // scales V in HSV space, keeps hue
float luminance(Color c);                 // 0.299 R + 0.587 G + 0.114 B

void toGrayscale(Canvas& canvas);

std::vector<Color> uniformPalette(int levelsR, int levelsG, int levelsB);
std::vector<Color> popularityPalette(const Canvas& canvas, int size);
Color nearestColor(Color c, const std::vector<Color>& palette);

void quantize(Canvas& canvas, const std::vector<Color>& palette);
// spread = distance between neighbouring palette levels (255 / (levels - 1)).
void ditherOrdered(Canvas& canvas, const std::vector<Color>& palette, float spread);
void ditherFloydSteinberg(Canvas& canvas, const std::vector<Color>& palette);

int countDistinctColors(const Canvas& canvas);
