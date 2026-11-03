#include "math/Mat4.h"

namespace math {

Mat4 Mat4::operator*(const Mat4& b) const {
    Mat4 r;
    for (int c = 0; c < 4; ++c)
        for (int row = 0; row < 4; ++row) {
            double s = 0;
            for (int k = 0; k < 4; ++k) s += m[k][row] * b.m[c][k];
            r.m[c][row] = s;
        }
    return r;
}

Vec4 Mat4::operator*(const Vec4& v) const {
    const double in[4] = {v.x, v.y, v.z, v.w};
    double out[4] = {0, 0, 0, 0};
    for (int row = 0; row < 4; ++row)
        for (int c = 0; c < 4; ++c) out[row] += m[c][row] * in[c];
    return {out[0], out[1], out[2], out[3]};
}

void Mat4::ToFloats(float out[16]) const {
    for (int c = 0; c < 4; ++c)
        for (int row = 0; row < 4; ++row) out[c * 4 + row] = static_cast<float>(m[c][row]);
}

Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    // Camera basis: forward f (towards the target), right s, true up u.
    const Vec3 f = Normalize(target - eye);
    const Vec3 s = Normalize(Cross(f, up));
    const Vec3 u = Cross(s, f);
    Mat4 r; // rows of the rotation are s, u, -f (OpenGL looks down -Z in eye space)
    r.m[0][0] = s.x;  r.m[1][0] = s.y;  r.m[2][0] = s.z;
    r.m[0][1] = u.x;  r.m[1][1] = u.y;  r.m[2][1] = u.z;
    r.m[0][2] = -f.x; r.m[1][2] = -f.y; r.m[2][2] = -f.z;
    r.m[3][0] = -Dot(s, eye);
    r.m[3][1] = -Dot(u, eye);
    r.m[3][2] = Dot(f, eye);
    return r;
}

Mat4 PerspectiveHorizontal(double fovX, double aspect, double n, double f) {
    const double pi = 3.14159265358979323846;
    const double sx = 1.0 / std::tan(fovX * pi / 360.0); // 1 / tan(fov/2)
    Mat4 r;
    r.m[0][0] = sx;
    r.m[1][1] = sx * aspect;           // same pixels per unit horizontally and vertically
    r.m[2][2] = -(f + n) / (f - n);
    r.m[2][3] = -1.0;
    r.m[3][2] = -2.0 * f * n / (f - n);
    r.m[3][3] = 0.0;
    return r;
}

Mat4 PixelToClip(int w, int h) {
    Mat4 r;
    r.m[0][0] = 2.0 / w;  r.m[3][0] = -1.0;   // x: 0..w  -> -1..1
    r.m[1][1] = -2.0 / h; r.m[3][1] = 1.0;    // y: 0..h (down) -> 1..-1
    return r;
}

bool ProjectToPixel(const Mat4& viewProj, const Vec3& p, int w, int h, double& px, double& py) {
    const Vec4 c = viewProj * Vec4{p.x, p.y, p.z, 1.0};
    if (c.w <= 0) return false;
    const double nx = c.x / c.w, ny = c.y / c.w;      // normalised device coordinates
    px = (nx + 1.0) * 0.5 * w;                        // window x (pixel centres at i + 0.5)
    py = (1.0 - ny) * 0.5 * h;                        // window y, top-down
    return true;
}

} // namespace math
