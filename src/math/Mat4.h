#pragma once
#include <cmath>

// Minimal 3D maths for the viewports: vectors, 4x4 matrices, camera matrices.
// Column-major like OpenGL (m[col][row]), right-handed, used with Max's Z-up world.
// No dependencies; unit-tested in tests/test_math.cpp.
namespace math {

struct Vec3 {
    double x = 0, y = 0, z = 0;
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
};

inline double Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 Cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline double Length(const Vec3& v) { return std::sqrt(Dot(v, v)); }
inline Vec3 Normalize(const Vec3& v) { const double l = Length(v); return l > 0 ? v * (1.0 / l) : v; }

struct Vec4 { double x = 0, y = 0, z = 0, w = 1; };

struct Mat4 {
    double m[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}; // identity

    Mat4 operator*(const Mat4& b) const;
    Vec4 operator*(const Vec4& v) const;
    void ToFloats(float out[16]) const; // column-major, for glUniformMatrix4fv
};

// Camera at `eye` looking at `target`; `up` is the world's up direction (Z for Max).
Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up);

// Perspective projection with a *horizontal* field of view (3ds Max's convention).
Mat4 PerspectiveHorizontal(double fovXDegrees, double aspect /* width/height */,
                           double zNear, double zFar);

// Maps window pixels (top-down, x right) of a w x h viewport to OpenGL clip space.
Mat4 PixelToClip(int w, int h);

// World point -> window pixel (top-down), through a view-projection matrix. Returns
// false if the point is behind the camera.
bool ProjectToPixel(const Mat4& viewProj, const Vec3& p, int w, int h, double& px, double& py);

// Whether an affine matrix mirrors (its 3 x 3 part has a negative determinant): faces
// then wind the other way round, so winding-based facing tests must flip (Phase 9.10).
inline bool Mirrors(const Mat4& m) {
    const Vec3 c0{m.m[0][0], m.m[0][1], m.m[0][2]}, c1{m.m[1][0], m.m[1][1], m.m[1][2]}, c2{m.m[2][0], m.m[2][1], m.m[2][2]};
    return Dot(Cross(c0, c1), c2) < 0;
}

} // namespace math
