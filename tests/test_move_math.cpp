// Unit tests for moving objects with axis constraints.
#include "scene/MoveMath.h"
#include "scene/Node.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(const math::Vec3& a, const math::Vec3& b) {
    return std::fabs(a.x - b.x) < 1e-9 && std::fabs(a.y - b.y) < 1e-9 && std::fabs(a.z - b.z) < 1e-9;
}

using namespace scene::move;

int main() {
    const bool xy[3] = {true, true, false}, x[3] = {true, false, false}, z[3] = {false, false, true};
    const bool all[3] = {true, true, true};
    // Constrain: keep the allowed axes only (e.g. Front view with XY: only X survives).
    CHECK(Near(Constrain({3, 4, 5}, xy), {3, 4, 0}));
    CHECK(Near(Constrain({3, 0, 5}, xy), {3, 0, 0}));
    CHECK(Near(Constrain({3, 4, 5}, z), {0, 0, 5}));

    // Drag planes: XY -> the ground (normal Z); X seen from the front-ish -> faces the camera.
    math::Vec3 n;
    CHECK(DragPlaneNormal(xy, {0, 1, -0.5}, n) && Near(n, {0, 0, 1}));
    CHECK(DragPlaneNormal(x, {0, 1, 0}, n) && std::fabs(n.x) < 1e-9 && std::fabs(std::fabs(n.y) - 1) < 1e-9);
    CHECK(DragPlaneNormal(z, {0.6, 0.8, 0}, n) && std::fabs(n.z) < 1e-9);     // contains Z, faces the view
    CHECK(!DragPlaneNormal(x, {1, 0, 0}, n));                                    // looking along X
    CHECK(!DragPlaneNormal(all, {0, 1, 0}, n));

    // Ray and plane.
    math::Vec3 hit;
    CHECK(RayPlane({0, 0, 10}, {0, 0, -1}, {0, 0, 2}, {0, 0, 1}, hit) && Near(hit, {0, 0, 2}));
    CHECK(!RayPlane({0, 0, 10}, {1, 0, 0}, {0, 0, 2}, {0, 0, 1}, hit));          // parallel
    CHECK(!RayPlane({0, 0, 10}, {0, 0, 1}, {0, 0, 2}, {0, 0, 1}, hit));          // behind

    // Into a parent's space: rotated 90° about Z, scaled 2, moved (10, 0, 0).
    scene::PRS parent;
    parent.position = {10, 0, 0};
    parent.rotation = math::Quat::FromAxisAngle({0, 0, 1}, 3.14159265358979323846 / 2);
    parent.scale = {2, 2, 2};
    const math::Mat4 pm = parent.Matrix();
    math::Vec3 local;
    CHECK(InverseTransformPoint(pm, math::TransformPoint(pm, {1, 2, 3}), local) && Near(local, {1, 2, 3}));
    CHECK(InverseTransformPoint(pm, {10, 2, 0}, local) && Near(local, {1, 0, 0}));  // world +2 along Y = local +1 along X
    // Rotation axis: one axis -> it; a plane -> its normal (XY -> Z); all -> none.
    const bool zx[3] = {true, false, true};
    CHECK(RotationAxis(x) == 0 && RotationAxis(xy) == 2 && RotationAxis(zx) == 1 && RotationAxis(all) == -1);
    // 180 px up = 90 degrees counter-clockwise about Z: X turns into Y.
    CHECK(Near(DragRotation(2, 180).Rotate({1, 0, 0}), {0, 1, 0}));
    CHECK(Near(DragRotation(-1, 180).Rotate({1, 0, 0}), {1, 0, 0}));
    // Under a parent turned 90 degrees about Z, a world turn about X is a local turn about
    // -Y: the node's world orientation must end up exactly world-delta * old.
    {
        const math::Quat parentRot = math::Quat::FromAxisAngle({0, 0, 1}, 3.14159265358979323846 / 2);
        const math::Quat start = math::Quat::FromAxisAngle({1, 0, 0}, 0.3);
        const math::Quat delta = DragRotation(0, 60);
        const math::Quat local = RotateInParent(parentRot, delta, start);
        const math::Vec3 v{0.2, 0.5, 0.7};
        CHECK(Near((parentRot * local).Rotate(v), (delta * parentRot * start).Rotate(v)));
    }
    // Scale: uniform; non-uniform (constrained axes only); squash keeps the volume.
    CHECK(Near(DragScale(ScaleMode::Uniform, xy, 50), {1.5, 1.5, 1.5}));
    CHECK(Near(DragScale(ScaleMode::NonUniform, xy, 50), {1.5, 1.5, 1}));
    {
        const math::Vec3 s = DragScale(ScaleMode::Squash, z, 100);       // Z doubles
        CHECK(std::fabs(s.z - 2) < 1e-12 && std::fabs(s.x * s.y * s.z - 1) < 1e-12 && std::fabs(s.x - s.y) < 1e-12);
    }
    CHECK(Near(DragScale(ScaleMode::Uniform, xy, -500), {0.01, 0.01, 0.01}));  // never below 1 %
    // Status bar values.
    CHECK(Near(RotationDegrees(2, 90), {0, 0, 45}) && Near(RotationDegrees(-1, 90), {0, 0, 0}));
    CHECK(Near(ScalePercent({1.5, 1, 0.25}), {150, 100, 25}));
    {
        const double r = 3.14159265358979323846 / 180;
        // X 30, then Y 20, then Z 40 (Z * Y * X as quaternions) comes back as (30, 20, 40).
        const math::Quat q = math::Quat::FromAxisAngle({0, 0, 1}, 40 * r) * math::Quat::FromAxisAngle({0, 1, 0}, 20 * r) *
                             math::Quat::FromAxisAngle({1, 0, 0}, 30 * r);
        const math::Vec3 e = EulerXYZDegrees(q);
        CHECK(std::fabs(e.x - 30) < 1e-9 && std::fabs(e.y - 20) < 1e-9 && std::fabs(e.z - 40) < 1e-9);
        CHECK(Near(EulerXYZDegrees({0, 0, 0, 1}), {0, 0, 0}));
    }
    if (failures == 0) std::printf("move math: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
