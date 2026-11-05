#pragma once
#include "math/Mat4.h"

// Rotations as unit quaternions, standard right-handed convention (a positive angle
// turns counter-clockwise when looking down the axis towards the origin).
// NOTE: 3ds Max *files* store the conjugate; use FromMaxFile when importing.
namespace math {

struct Quat {
    double x = 0, y = 0, z = 0, w = 1;   // identity

    static Quat FromAxisAngle(const Vec3& axis, double radians);
    // A rotation as stored in a .max file (left-handed): conjugate it (max-format.md §5.2).
    static Quat FromMaxFile(double x, double y, double z, double w) { return Quat{-x, -y, -z, w}.Normalized(); }

    Quat operator*(const Quat& b) const;           // apply b first, then this
    Quat Conjugate() const { return {-x, -y, -z, w}; }
    Quat Normalized() const;
    Vec3 Rotate(const Vec3& v) const;
    Mat4 ToMat4() const;
    double AngleAbout(const Vec3& axis) const;     // signed angle, radians (for tests and UI)
};

// Affine building blocks (column vectors: M * p).
Mat4 Translation(const Vec3& t);
Mat4 Scaling(const Vec3& s);
Vec3 TransformPoint(const Mat4& m, const Vec3& p);
Vec3 TransformDirection(const Mat4& m, const Vec3& d);
Mat4 AffineInverse(const Mat4& m);                 // inverse of rotation/scale + translation

} // namespace math
