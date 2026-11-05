#pragma once
// Geometric transformations with homogeneous coordinates.
//   - Mat3: 2D transforms (translate, rotate, scale, shear, reflect, composites)
//   - Mat4: 3D transforms (translate, rotate X/Y/Z, scale) and the viewing
//           pipeline (look-at view matrix, perspective projection)
// Points are column vectors: p' = M * p. Composite transforms are built by
// multiplication and apply right-to-left: (T * R) * p rotates first.
#include <cmath>

#include "core/Geometry.h"

constexpr float PI = 3.14159265358979f;
inline float radians(float deg) { return deg * PI / 180.0f; }

struct Vec3 {
    float x = 0, y = 0, z = 0;
};
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline float length(Vec3 v) { return std::sqrt(dot(v, v)); }
inline Vec3 normalize(Vec3 v) { float l = length(v); return l == 0 ? Vec3{} : v * (1.0f / l); }

struct Vec4 {
    float x = 0, y = 0, z = 0, w = 1;
};

struct Mat3 {
    float m[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    static Mat3 identity() { return {}; }
    static Mat3 translate(float tx, float ty);
    static Mat3 rotate(float radians);   // positive = clockwise on screen (y points down)
    static Mat3 scale(float sx, float sy);
    static Mat3 shear(float shx, float shy);
    static Mat3 reflectX();               // mirror across the x axis (y -> -y)
    static Mat3 reflectY();               // mirror across the y axis (x -> -x)
    // Composites about an arbitrary fixed point: T(p) * M * T(-p).
    static Mat3 rotateAbout(Vec2 pivot, float radians);
    static Mat3 scaleAbout(Vec2 pivot, float sx, float sy);
    static Mat3 reflectAboutHorizontal(float lineY);
    static Mat3 reflectAboutVertical(float lineX);

    Mat3 operator*(const Mat3& o) const;
    Vec2 apply(Vec2 p) const;
};

Polygon transformed(const Polygon& poly, const Mat3& m);

struct Mat4 {
    float m[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};

    static Mat4 identity() { return {}; }
    static Mat4 translate(Vec3 t);
    static Mat4 scale(Vec3 s);
    static Mat4 rotateX(float radians);
    static Mat4 rotateY(float radians);
    static Mat4 rotateZ(float radians);
    // Camera at eye looking at target (right-handed, camera looks down -z).
    static Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up);
    // OpenGL-style projection: maps the view frustum to the cube [-1,1]^3.
    static Mat4 perspective(float fovYRadians, float aspect, float zNear, float zFar);
    static Mat4 orthographic(float left, float right, float bottom, float top, float zNear, float zFar);

    Mat4 operator*(const Mat4& o) const;
    Vec4 apply(Vec4 v) const;
    Vec3 applyPoint(Vec3 p) const;    // includes the perspective divide
    Vec3 applyVector(Vec3 v) const;   // w = 0: ignores translation
    Mat4 inverse() const;
    const float* data() const { return &m[0][0]; }   // row-major (upload with transpose = GL_TRUE)
};
