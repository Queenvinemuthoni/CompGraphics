#pragma once
// Animation building blocks.
//   - Easing functions map linear time t in [0,1] to a shaped progress value
//   - Track<T>: keyframes + interpolation (tweening) for floats, points, colours
//   - Day/night cycle: one time-of-day value drives sun position, sky colours
//     and lighting; shared by the pure C++ animations and the OpenGL engine
#include <algorithm>
#include <string>
#include <vector>

#include "core/Canvas.h"
#include "core/Geometry.h"
#include "math/Transform.h"

namespace ease {
float linear(float t);
float inQuad(float t);
float outQuad(float t);
float inOutQuad(float t);
float inOutCubic(float t);
float outBack(float t);     // overshoots slightly, then settles (block "pop")
float outBounce(float t);   // bounces like a dropped object
}  // namespace ease

using EaseFn = float (*)(float);

inline float tween(float a, float b, float t) { return a + (b - a) * t; }
inline Vec2 tween(Vec2 a, Vec2 b, float t) { return lerp(a, b, t); }
inline Vec3 tween(Vec3 a, Vec3 b, float t) { return a + (b - a) * t; }
inline Color tween(Color a, Color b, float t) { return lerpColor(a, b, std::clamp(t, 0.0f, 1.0f)); }

// Progress of an event that starts at `start` and lasts `duration`, clamped to [0,1].
inline float progress(float time, float start, float duration) {
    return std::clamp((time - start) / duration, 0.0f, 1.0f);
}

template <typename T>
class Track {
public:
    Track& key(float time, T value, EaseFn easeIn = ease::linear) {
        keys_.push_back({time, value, easeIn});
        std::sort(keys_.begin(), keys_.end(), [](const Key& a, const Key& b) { return a.time < b.time; });
        return *this;
    }

    // Value at `time`; each segment uses the easing of the key it moves towards.
    T sample(float time) const {
        if (keys_.empty()) return T{};
        if (time <= keys_.front().time) return keys_.front().value;
        if (time >= keys_.back().time) return keys_.back().value;
        for (size_t i = 0; i + 1 < keys_.size(); ++i) {
            const Key& a = keys_[i];
            const Key& b = keys_[i + 1];
            if (time <= b.time) {
                float t = b.time > a.time ? (time - a.time) / (b.time - a.time) : 1.0f;
                return tween(a.value, b.value, b.ease(t));
            }
        }
        return keys_.back().value;
    }

private:
    struct Key {
        float time;
        T value;
        EaseFn ease;
    };
    std::vector<Key> keys_;
};

struct DayNight {
    float sunAngle;      // radians; 0 = sunrise (east), PI/2 = noon, PI = sunset
    Vec3 sunDir;         // unit vector pointing towards the sun
    Color skyTop, skyBottom;
    Color lightTint;     // multiplies lit surfaces
    float ambient;       // 0..1
    float daylight;      // 0 at night .. 1 at noon
    float starAlpha;     // 0 by day .. 1 at night
};

// timeOfDay in [0,1): 0 = sunrise, 0.25 = noon, 0.5 = sunset, 0.75 = midnight.
DayNight dayNight(float timeOfDay);
// Clock text such as "06:00" for a time of day.
std::string clockText(float timeOfDay);
