#include "math/Quat.h"
#include <cmath>

namespace math {

Quat Quat::FromAxisAngle(const Vec3& axis, double r) {
    const Vec3 a = Normalize(axis);
    const double s = std::sin(r / 2);
    return {a.x * s, a.y * s, a.z * s, std::cos(r / 2)};
}

Quat Quat::operator*(const Quat& b) const {
    return {w * b.x + x * b.w + y * b.z - z * b.y,
            w * b.y - x * b.z + y * b.w + z * b.x,
            w * b.z + x * b.y - y * b.x + z * b.w,
            w * b.w - x * b.x - y * b.y - z * b.z};
}

Quat Quat::Normalized() const {
    const double l = std::sqrt(x * x + y * y + z * z + w * w);
    return l > 0 ? Quat{x / l, y / l, z / l, w / l} : Quat{};
}

Vec3 Quat::Rotate(const Vec3& v) const {
    const Quat p{v.x, v.y, v.z, 0};
    const Quat r = (*this) * p * Conjugate();
    return {r.x, r.y, r.z};
}

Mat4 Quat::ToMat4() const {
    const Vec3 cx = Rotate({1, 0, 0}), cy = Rotate({0, 1, 0}), cz = Rotate({0, 0, 1});
    Mat4 m;
    m.m[0][0] = cx.x; m.m[0][1] = cx.y; m.m[0][2] = cx.z;
    m.m[1][0] = cy.x; m.m[1][1] = cy.y; m.m[1][2] = cy.z;
    m.m[2][0] = cz.x; m.m[2][1] = cz.y; m.m[2][2] = cz.z;
    return m;
}

double Quat::AngleAbout(const Vec3& axis) const {
    // Twist about `axis`: project the vector part onto it.
    const Vec3 a = Normalize(axis);
    const double p = x * a.x + y * a.y + z * a.z;
    return 2.0 * std::atan2(p, w);
}

Mat4 Translation(const Vec3& t) {
    Mat4 m;
    m.m[3][0] = t.x; m.m[3][1] = t.y; m.m[3][2] = t.z;
    return m;
}

Mat4 Scaling(const Vec3& s) {
    Mat4 m;
    m.m[0][0] = s.x; m.m[1][1] = s.y; m.m[2][2] = s.z;
    return m;
}

Vec3 TransformPoint(const Mat4& m, const Vec3& p) {
    const Vec4 r = m * Vec4{p.x, p.y, p.z, 1};
    return {r.x, r.y, r.z};
}

Vec3 TransformDirection(const Mat4& m, const Vec3& d) {
    const Vec4 r = m * Vec4{d.x, d.y, d.z, 0};
    return {r.x, r.y, r.z};
}

Mat4 AffineInverse(const Mat4& m) {
    // Invert the 3x3 part (general, allows non-uniform scale), then the translation.
    const double a = m.m[0][0], b = m.m[1][0], c = m.m[2][0];
    const double d = m.m[0][1], e = m.m[1][1], f = m.m[2][1];
    const double g = m.m[0][2], h = m.m[1][2], i = m.m[2][2];
    const double A = e * i - f * h, B = -(d * i - f * g), C = d * h - e * g;
    const double det = a * A + b * B + c * C;
    Mat4 r;
    if (std::fabs(det) < 1e-300) return r;
    const double k = 1.0 / det;
    r.m[0][0] = A * k;                 r.m[1][0] = -(b * i - c * h) * k; r.m[2][0] = (b * f - c * e) * k;
    r.m[0][1] = B * k;                 r.m[1][1] = (a * i - c * g) * k;  r.m[2][1] = -(a * f - c * d) * k;
    r.m[0][2] = C * k;                 r.m[1][2] = -(a * h - b * g) * k; r.m[2][2] = (a * e - b * d) * k;
    const Vec3 t{m.m[3][0], m.m[3][1], m.m[3][2]};
    const Vec3 it = TransformDirection(r, t);
    r.m[3][0] = -it.x; r.m[3][1] = -it.y; r.m[3][2] = -it.z;
    return r;
}

} // namespace math
