#include "color/ColorOps.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

#include "core/Geometry.h"

HSV rgbToHsv(Color c) {
    float r = c.r / 255.0f, g = c.g / 255.0f, b = c.b / 255.0f;
    float mx = std::max({r, g, b}), mn = std::min({r, g, b}), d = mx - mn;
    float h = 0;
    if (d > 0) {
        if (mx == r)      h = 60.0f * std::fmod((g - b) / d, 6.0f);
        else if (mx == g) h = 60.0f * ((b - r) / d + 2.0f);
        else              h = 60.0f * ((r - g) / d + 4.0f);
    }
    if (h < 0) h += 360.0f;
    return {h, mx == 0 ? 0 : d / mx, mx};
}

Color hsvToRgb(HSV hsv) {
    float c = hsv.v * hsv.s;
    float hp = std::fmod(hsv.h, 360.0f) / 60.0f;
    float x = c * (1 - std::fabs(std::fmod(hp, 2.0f) - 1));
    float r = 0, g = 0, b = 0;
    if (hp < 1)      { r = c; g = x; }
    else if (hp < 2) { r = x; g = c; }
    else if (hp < 3) { g = c; b = x; }
    else if (hp < 4) { g = x; b = c; }
    else if (hp < 5) { r = x; b = c; }
    else             { r = c; b = x; }
    float m = hsv.v - c;
    return rgb(iround((r + m) * 255), iround((g + m) * 255), iround((b + m) * 255));
}

Color shade(Color c, float valueFactor) {
    HSV hsv = rgbToHsv(c);
    hsv.v = std::clamp(hsv.v * valueFactor, 0.0f, 1.0f);
    return hsvToRgb(hsv);
}

float luminance(Color c) { return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b; }

void toGrayscale(Canvas& canvas) {
    for (int y = 0; y < canvas.height(); ++y)
        for (int x = 0; x < canvas.width(); ++x) {
            int l = iround(luminance(canvas.getPixel(x, y)));
            canvas.setPixel(x, y, rgb(l, l, l));
        }
}

std::vector<Color> uniformPalette(int levelsR, int levelsG, int levelsB) {
    std::vector<Color> pal;
    for (int r = 0; r < levelsR; ++r)
        for (int g = 0; g < levelsG; ++g)
            for (int b = 0; b < levelsB; ++b)
                pal.push_back(rgb(r * 255 / (levelsR - 1), g * 255 / (levelsG - 1), b * 255 / (levelsB - 1)));
    return pal;
}

std::vector<Color> popularityPalette(const Canvas& canvas, int size) {
    // Histogram over a 4-4-4 bit reduced colour cube, then keep the most popular bins.
    std::unordered_map<int, int> histogram;
    for (int y = 0; y < canvas.height(); ++y)
        for (int x = 0; x < canvas.width(); ++x) {
            Color c = canvas.getPixel(x, y);
            ++histogram[(c.r >> 4) << 8 | (c.g >> 4) << 4 | (c.b >> 4)];
        }
    std::vector<std::pair<int, int>> bins(histogram.begin(), histogram.end());
    std::sort(bins.begin(), bins.end(), [](auto& a, auto& b) { return a.second > b.second; });
    std::vector<Color> pal;
    for (int i = 0; i < size && i < static_cast<int>(bins.size()); ++i) {
        int k = bins[i].first;
        pal.push_back(rgb(((k >> 8) & 15) * 16 + 8, ((k >> 4) & 15) * 16 + 8, (k & 15) * 16 + 8));
    }
    return pal;
}

Color nearestColor(Color c, const std::vector<Color>& palette) {
    Color best = palette.front();
    int bestDist = INT_MAX;
    for (const Color& p : palette) {
        int dr = c.r - p.r, dg = c.g - p.g, db = c.b - p.b;
        int d = 2 * dr * dr + 4 * dg * dg + 3 * db * db;  // weighted: the eye is most sensitive to green
        if (d < bestDist) { bestDist = d; best = p; }
    }
    return best;
}

void quantize(Canvas& canvas, const std::vector<Color>& palette) {
    for (int y = 0; y < canvas.height(); ++y)
        for (int x = 0; x < canvas.width(); ++x)
            canvas.setPixel(x, y, nearestColor(canvas.getPixel(x, y), palette));
}

void ditherOrdered(Canvas& canvas, const std::vector<Color>& palette, float spread) {
    static const int bayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
    for (int y = 0; y < canvas.height(); ++y)
        for (int x = 0; x < canvas.width(); ++x) {
            float offset = ((bayer[y & 3][x & 3] + 0.5f) / 16.0f - 0.5f) * spread;
            Color c = canvas.getPixel(x, y);
            Color biased = rgb(iround(c.r + offset), iround(c.g + offset), iround(c.b + offset));
            canvas.setPixel(x, y, nearestColor(biased, palette));
        }
}

void ditherFloydSteinberg(Canvas& canvas, const std::vector<Color>& palette) {
    int w = canvas.width(), h = canvas.height();
    // Work in floats so the diffused error can push values past 0..255.
    std::vector<float> buf(static_cast<size_t>(w) * h * 3);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            Color c = canvas.getPixel(x, y);
            float* p = &buf[(static_cast<size_t>(y) * w + x) * 3];
            p[0] = c.r; p[1] = c.g; p[2] = c.b;
        }
    auto spread = [&](int x, int y, const float err[3], float weight) {
        if (x < 0 || x >= w || y >= h) return;
        float* p = &buf[(static_cast<size_t>(y) * w + x) * 3];
        for (int k = 0; k < 3; ++k) p[k] += err[k] * weight;
    };
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            float* p = &buf[(static_cast<size_t>(y) * w + x) * 3];
            Color old = rgb(iround(p[0]), iround(p[1]), iround(p[2]));
            Color chosen = nearestColor(old, palette);
            canvas.setPixel(x, y, chosen);
            float err[3] = {p[0] - chosen.r, p[1] - chosen.g, p[2] - chosen.b};
            spread(x + 1, y, err, 7 / 16.0f);
            spread(x - 1, y + 1, err, 3 / 16.0f);
            spread(x, y + 1, err, 5 / 16.0f);
            spread(x + 1, y + 1, err, 1 / 16.0f);
        }
}

int countDistinctColors(const Canvas& canvas) {
    std::unordered_set<int> seen;
    for (int y = 0; y < canvas.height(); ++y)
        for (int x = 0; x < canvas.width(); ++x) {
            Color c = canvas.getPixel(x, y);
            seen.insert(c.r << 16 | c.g << 8 | c.b);
        }
    return static_cast<int>(seen.size());
}
