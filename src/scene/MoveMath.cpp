#include "scene/MoveMath.h"
#include <algorithm>
#include <cmath>

namespace scene::move {

math::Vec3 Constrain(const math::Vec3& d, const bool active[3]) {
    return {active[0] ? d.x : 0.0, active[1] ? d.y : 0.0, active[2] ? d.z : 0.0};
}

bool DragPlaneNormal(const bool active[3], const math::Vec3& viewDir, math::Vec3& normal) {
    const math::Vec3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const int count = int(active[0]) + int(active[1]) + int(active[2]);
    if (count == 2) {
        for (int a = 0; a < 3; ++a)
            if (!active[a]) normal = axes[a];
        return true;
    }
    if (count != 1) return false;
    const math::Vec3 axis = active[0] ? axes[0] : active[1] ? axes[1] : axes[2];
    const math::Vec3 n = math::Cross(axis, math::Cross(axis, viewDir));
    if (math::Length(n) < 1e-9) return false;          // looking straight along the axis
    normal = math::Normalize(n);
    return true;
}

bool RayPlane(const math::Vec3& o, const math::Vec3& dir, const math::Vec3& point, const math::Vec3& n,
              math::Vec3& hit) {
    const double denom = math::Dot(dir, n);
    if (std::fabs(denom) < 1e-12) return false;
    const double t = math::Dot(point - o, n) / denom;
    if (t < 0) return false;
    hit = o + dir * t;
    return true;
}

bool InverseTransformPoint(const math::Mat4& m, const math::Vec3& world, math::Vec3& local) {
    // Columns of the 3x3 part and the translation, from how the matrix maps points.
    const math::Vec3 t = math::TransformPoint(m, {0, 0, 0});
    const math::Vec3 c0 = math::TransformPoint(m, {1, 0, 0}) - t;
    const math::Vec3 c1 = math::TransformPoint(m, {0, 1, 0}) - t;
    const math::Vec3 c2 = math::TransformPoint(m, {0, 0, 1}) - t;
    const double det = math::Dot(c0, math::Cross(c1, c2));
    if (std::fabs(det) < 1e-15) return false;
    const math::Vec3 p = world - t;
    // Cramer's rule.
    local = {math::Dot(p, math::Cross(c1, c2)) / det, math::Dot(c0, math::Cross(p, c2)) / det,
             math::Dot(c0, math::Cross(c1, p)) / det};
    return true;
}

int RotationAxis(const bool active[3]) {
    const int count = int(active[0]) + int(active[1]) + int(active[2]);
    for (int a = 0; a < 3; ++a) {
        if (count == 1 && active[a]) return a;
        if (count == 2 && !active[a]) return a;
    }
    return -1;
}

math::Quat DragRotation(int axis, int upPixels) {
    if (axis < 0) return {0, 0, 0, 1};
    const math::Vec3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    return math::Quat::FromAxisAngle(axes[axis], upPixels * 0.5 * 3.14159265358979323846 / 180.0);
}

math::Quat RotateInParent(const math::Quat& parentWorld, const math::Quat& worldDelta, const math::Quat& start) {
    // Express the world rotation in the parent's frame, then apply it after the start.
    return (parentWorld.Conjugate() * worldDelta * parentWorld * start).Normalized();
}

math::Vec3 DragScale(ScaleMode mode, const bool active[3], int upPixels) {
    const double f = std::max(0.01, 1.0 + upPixels * 0.01);
    if (mode == ScaleMode::Uniform) return {f, f, f};
    double s[3];
    const int n = int(active[0]) + int(active[1]) + int(active[2]);
    const double other = (mode == ScaleMode::Squash && n > 0 && n < 3) ? std::pow(f, -double(n) / (3 - n)) : 1.0;
    for (int a = 0; a < 3; ++a) s[a] = active[a] ? f : other;
    return {s[0], s[1], s[2]};
}

math::Vec3 RotationDegrees(int axis, int upPixels) {
    double d[3] = {0, 0, 0};
    if (axis >= 0) d[axis] = upPixels * 0.5;
    return {d[0], d[1], d[2]};
}

math::Vec3 ScalePercent(const math::Vec3& f) { return {f.x * 100, f.y * 100, f.z * 100}; }

math::Vec3 EulerXYZDegrees(const math::Quat& q0) {
    const math::Quat q = q0.Normalized();
    const double k = 180.0 / 3.14159265358979323846;
    // R = Rz * Ry * Rx; standard extraction.
    const double sinX = 2 * (q.w * q.x + q.y * q.z), cosX = 1 - 2 * (q.x * q.x + q.y * q.y);
    const double sinY = std::clamp(2 * (q.w * q.y - q.z * q.x), -1.0, 1.0);
    const double sinZ = 2 * (q.w * q.z + q.x * q.y), cosZ = 1 - 2 * (q.y * q.y + q.z * q.z);
    return {std::atan2(sinX, cosX) * k, std::asin(sinY) * k, std::atan2(sinZ, cosZ) * k};
}

namespace {
math::Vec3 ToFrame(const Frame& f, const math::Vec3& v) { return {math::Dot(v, f.axis[0]), math::Dot(v, f.axis[1]), math::Dot(v, f.axis[2])}; }
math::Vec3 FromFrame(const Frame& f, const math::Vec3& v) { return f.axis[0] * v.x + f.axis[1] * v.y + f.axis[2] * v.z; }
} // namespace

math::Vec3 ConstrainInFrame(const math::Vec3& delta, const bool active[3], const Frame& f) {
    return FromFrame(f, Constrain(ToFrame(f, delta), active));
}

bool DragPlaneNormalInFrame(const bool active[3], const math::Vec3& viewDir, const Frame& f, math::Vec3& normal) {
    math::Vec3 n;
    if (!DragPlaneNormal(active, ToFrame(f, viewDir), n)) return false;
    normal = FromFrame(f, n);
    return true;
}

math::Quat DragRotationInFrame(int axis, int upPixels, const Frame& f) {
    if (axis < 0 || axis > 2) return math::Quat{};
    return math::Quat::FromAxisAngle(f.axis[axis], upPixels * 0.5 * 3.14159265358979323846 / 180.0);
}

Frame MakeFrame(RefCoord system, bool perspective, const math::Vec3& right, const math::Vec3& up,
                const math::Quat* parentRotation, const math::Vec3& parentPivot,
                const math::Quat* localRotation, const math::Vec3& localPivot) {
    Frame f;
    auto rotated = [&](const math::Quat& q, const math::Vec3& origin) {
        for (int a = 0; a < 3; ++a) f.axis[a] = math::Normalize(q.Rotate(f.axis[a]));
        f.origin = origin;
    };
    switch (system) {
    case RefCoord::View:
        if (perspective) break;                                       // World
        [[fallthrough]];
    case RefCoord::Screen:
        f.axis[0] = math::Normalize(right);
        f.axis[1] = math::Normalize(up);
        f.axis[2] = math::Normalize(math::Cross(right, up));         // out of the screen
        break;
    case RefCoord::Parent:
        if (parentRotation) rotated(*parentRotation, parentPivot);   // no parent: the world
        break;
    case RefCoord::Local:
        if (localRotation) rotated(*localRotation, localPivot);
        break;
    case RefCoord::World: case RefCoord::Grid: case RefCoord::Pick:
        break;
    }
    return f;
}

} // namespace scene::move
