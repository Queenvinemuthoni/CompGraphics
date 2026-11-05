#include "anim/Animation.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace ease {

float linear(float t) { return t; }
float inQuad(float t) { return t * t; }
float outQuad(float t) { return 1 - (1 - t) * (1 - t); }
float inOutQuad(float t) { return t < 0.5f ? 2 * t * t : 1 - 2 * (1 - t) * (1 - t); }
float inOutCubic(float t) { return t < 0.5f ? 4 * t * t * t : 1 - 4 * (1 - t) * (1 - t) * (1 - t); }

float outBack(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1;
    float u = t - 1;
    return 1 + c3 * u * u * u + c1 * u * u;
}

float outBounce(float t) {
    const float n = 7.5625f, d = 2.75f;
    if (t < 1 / d) return n * t * t;
    if (t < 2 / d) { t -= 1.5f / d; return n * t * t + 0.75f; }
    if (t < 2.5f / d) { t -= 2.25f / d; return n * t * t + 0.9375f; }
    t -= 2.625f / d;
    return n * t * t + 0.984375f;
}

}  // namespace ease

DayNight dayNight(float timeOfDay) {
    float t = timeOfDay - std::floor(timeOfDay);

    // Sky colour keyframes around the clock (wraps from 1.0 back to 0.0).
    static const Track<Color> top = Track<Color>()
        .key(0.00f, {250, 150, 90}).key(0.08f, {90, 150, 230}, ease::inOutQuad)
        .key(0.25f, {70, 136, 226}).key(0.42f, {96, 150, 220})
        .key(0.50f, {236, 120, 80}, ease::inOutQuad).key(0.56f, {70, 50, 110}, ease::inOutQuad)
        .key(0.75f, {10, 14, 34}).key(0.94f, {30, 30, 70}).key(1.00f, {250, 150, 90}, ease::inOutQuad);
    static const Track<Color> bottom = Track<Color>()
        .key(0.00f, {255, 206, 150}).key(0.08f, {190, 222, 248}, ease::inOutQuad)
        .key(0.25f, {196, 226, 250}).key(0.42f, {206, 226, 244})
        .key(0.50f, {255, 186, 120}, ease::inOutQuad).key(0.56f, {150, 90, 120}, ease::inOutQuad)
        .key(0.75f, {24, 30, 60}).key(0.94f, {60, 50, 90}).key(1.00f, {255, 206, 150}, ease::inOutQuad);
    static const Track<Color> tint = Track<Color>()
        .key(0.00f, {255, 196, 160}).key(0.08f, {255, 255, 255}, ease::inOutQuad)
        .key(0.42f, {255, 255, 255}).key(0.50f, {255, 180, 130}, ease::inOutQuad)
        .key(0.58f, {120, 120, 190}, ease::inOutQuad).key(0.75f, {90, 100, 170})
        .key(0.94f, {120, 120, 190}).key(1.00f, {255, 196, 160}, ease::inOutQuad);

    DayNight d;
    d.sunAngle = t * 2 * PI;
    // The sun travels east -> overhead -> west, tilted slightly towards +z.
    d.sunDir = normalize({std::cos(d.sunAngle), std::sin(d.sunAngle), 0.35f});
    d.skyTop = top.sample(t);
    d.skyBottom = bottom.sample(t);
    d.lightTint = tint.sample(t);
    float elevation = std::sin(d.sunAngle);
    d.daylight = std::clamp(elevation * 2.5f + 0.2f, 0.0f, 1.0f);
    d.ambient = 0.28f + 0.30f * d.daylight;
    d.starAlpha = std::clamp(-elevation * 3.0f, 0.0f, 1.0f);
    return d;
}

std::string clockText(float timeOfDay) {
    float t = timeOfDay - std::floor(timeOfDay);
    int minutes = static_cast<int>((t * 24 + 6) * 60) % (24 * 60);   // t = 0 is 06:00
    char buf[8];
    std::snprintf(buf, sizeof buf, "%02d:%02d", minutes / 60, minutes % 60);
    return buf;
}
