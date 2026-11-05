#include "math/Transform.h"

#include <utility>

Mat3 Mat3::translate(float tx, float ty) {
    Mat3 r;
    r.m[0][2] = tx;
    r.m[1][2] = ty;
    return r;
}

Mat3 Mat3::rotate(float a) {
    Mat3 r;
    float c = std::cos(a), s = std::sin(a);
    r.m[0][0] = c; r.m[0][1] = -s;
    r.m[1][0] = s; r.m[1][1] = c;
    return r;
}

Mat3 Mat3::scale(float sx, float sy) {
    Mat3 r;
    r.m[0][0] = sx;
    r.m[1][1] = sy;
    return r;
}

Mat3 Mat3::shear(float shx, float shy) {
    Mat3 r;
    r.m[0][1] = shx;
    r.m[1][0] = shy;
    return r;
}

Mat3 Mat3::reflectX() { return scale(1, -1); }
Mat3 Mat3::reflectY() { return scale(-1, 1); }

Mat3 Mat3::rotateAbout(Vec2 p, float a) { return translate(p.x, p.y) * rotate(a) * translate(-p.x, -p.y); }
Mat3 Mat3::scaleAbout(Vec2 p, float sx, float sy) { return translate(p.x, p.y) * scale(sx, sy) * translate(-p.x, -p.y); }
Mat3 Mat3::reflectAboutHorizontal(float y) { return translate(0, y) * reflectX() * translate(0, -y); }
Mat3 Mat3::reflectAboutVertical(float x) { return translate(x, 0) * reflectY() * translate(-x, 0); }

Mat3 Mat3::operator*(const Mat3& o) const {
    Mat3 r;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            r.m[i][j] = 0;
            for (int k = 0; k < 3; ++k) r.m[i][j] += m[i][k] * o.m[k][j];
        }
    return r;
}

Vec2 Mat3::apply(Vec2 p) const {
    float x = m[0][0] * p.x + m[0][1] * p.y + m[0][2];
    float y = m[1][0] * p.x + m[1][1] * p.y + m[1][2];
    float w = m[2][0] * p.x + m[2][1] * p.y + m[2][2];
    return {x / w, y / w};
}

Polygon transformed(const Polygon& poly, const Mat3& m) {
    Polygon out;
    out.reserve(poly.size());
    for (const Vec2& p : poly) out.push_back(m.apply(p));
    return out;
}

Mat4 Mat4::translate(Vec3 t) {
    Mat4 r;
    r.m[0][3] = t.x; r.m[1][3] = t.y; r.m[2][3] = t.z;
    return r;
}

Mat4 Mat4::scale(Vec3 s) {
    Mat4 r;
    r.m[0][0] = s.x; r.m[1][1] = s.y; r.m[2][2] = s.z;
    return r;
}

Mat4 Mat4::rotateX(float a) {
    Mat4 r;
    float c = std::cos(a), s = std::sin(a);
    r.m[1][1] = c; r.m[1][2] = -s;
    r.m[2][1] = s; r.m[2][2] = c;
    return r;
}

Mat4 Mat4::rotateY(float a) {
    Mat4 r;
    float c = std::cos(a), s = std::sin(a);
    r.m[0][0] = c;  r.m[0][2] = s;
    r.m[2][0] = -s; r.m[2][2] = c;
    return r;
}

Mat4 Mat4::rotateZ(float a) {
    Mat4 r;
    float c = std::cos(a), s = std::sin(a);
    r.m[0][0] = c; r.m[0][1] = -s;
    r.m[1][0] = s; r.m[1][1] = c;
    return r;
}

Mat4 Mat4::lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = normalize(target - eye);    // forward
    Vec3 s = normalize(cross(f, up));    // right
    Vec3 u = cross(s, f);                // true up
    Mat4 r;
    r.m[0][0] = s.x;  r.m[0][1] = s.y;  r.m[0][2] = s.z;  r.m[0][3] = -dot(s, eye);
    r.m[1][0] = u.x;  r.m[1][1] = u.y;  r.m[1][2] = u.z;  r.m[1][3] = -dot(u, eye);
    r.m[2][0] = -f.x; r.m[2][1] = -f.y; r.m[2][2] = -f.z; r.m[2][3] = dot(f, eye);
    return r;
}

Mat4 Mat4::perspective(float fovY, float aspect, float n, float f) {
    Mat4 r;
    float t = 1.0f / std::tan(fovY / 2);
    r.m[0][0] = t / aspect;
    r.m[1][1] = t;
    r.m[2][2] = (f + n) / (n - f);
    r.m[2][3] = 2 * f * n / (n - f);
    r.m[3][2] = -1;
    r.m[3][3] = 0;
    return r;
}

Mat4 Mat4::orthographic(float l, float r_, float b, float t, float n, float f) {
    Mat4 r;
    r.m[0][0] = 2 / (r_ - l); r.m[0][3] = -(r_ + l) / (r_ - l);
    r.m[1][1] = 2 / (t - b);  r.m[1][3] = -(t + b) / (t - b);
    r.m[2][2] = -2 / (f - n); r.m[2][3] = -(f + n) / (f - n);
    return r;
}

Mat4 Mat4::operator*(const Mat4& o) const {
    Mat4 r;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) {
            r.m[i][j] = 0;
            for (int k = 0; k < 4; ++k) r.m[i][j] += m[i][k] * o.m[k][j];
        }
    return r;
}

Vec4 Mat4::apply(Vec4 v) const {
    return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
            m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w};
}

Vec3 Mat4::applyPoint(Vec3 p) const {
    Vec4 v = apply({p.x, p.y, p.z, 1});
    return {v.x / v.w, v.y / v.w, v.z / v.w};
}

Vec3 Mat4::applyVector(Vec3 v) const {
    Vec4 r = apply({v.x, v.y, v.z, 0});
    return {r.x, r.y, r.z};
}

Mat4 Mat4::inverse() const {
    // Gauss-Jordan elimination with partial pivoting.
    float a[4][8];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) {
            a[i][j] = m[i][j];
            a[i][j + 4] = i == j ? 1.0f : 0.0f;
        }
    for (int col = 0; col < 4; ++col) {
        int pivot = col;
        for (int r = col + 1; r < 4; ++r)
            if (std::fabs(a[r][col]) > std::fabs(a[pivot][col])) pivot = r;
        if (a[pivot][col] == 0) return Mat4{};   // singular: fall back to identity
        for (int j = 0; j < 8; ++j) std::swap(a[col][j], a[pivot][j]);
        float d = a[col][col];
        for (int j = 0; j < 8; ++j) a[col][j] /= d;
        for (int r = 0; r < 4; ++r) {
            if (r == col) continue;
            float f = a[r][col];
            for (int j = 0; j < 8; ++j) a[r][j] -= f * a[col][j];
        }
    }
    Mat4 inv;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) inv.m[i][j] = a[i][j + 4];
    return inv;
}
