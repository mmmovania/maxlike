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

} // namespace scene::move
