#pragma once
#include "math/Mat4.h"
#include "math/Quat.h"

// The geometry of moving objects with an axis constraint. No Windows dependency
// (unit-tested in tests/test_move_math.cpp).
namespace scene::move {

// Keep only the components of a world-space movement allowed by the constraint
// (active[a]: world axis a may change).
math::Vec3 Constrain(const math::Vec3& delta, const bool active[3]);

// The plane a perspective drag moves on, through the pivot: for a two-axis constraint
// its own plane (normal = the third axis); for one axis, the plane containing that axis
// that faces the camera most (normal = axis × (axis × view direction)). False for
// constraints that give no plane (all or no axes, or an axis along the view).
bool DragPlaneNormal(const bool active[3], const math::Vec3& viewDir, math::Vec3& normal);

// Where a ray meets a plane; false if parallel or behind the ray's origin.
bool RayPlane(const math::Vec3& origin, const math::Vec3& dir, const math::Vec3& point,
              const math::Vec3& normal, math::Vec3& hit);

// A world point in the space of an affine matrix (its inverse applied): converts a
// node's new world position into its parent's space. False if the matrix is singular.
bool InverseTransformPoint(const math::Mat4& m, const math::Vec3& world, math::Vec3& local);

// Rotate and Scale (NOTE: R3's rules and speeds are from memory, unverified).

// The world axis Rotate turns about: a single-axis constraint's axis; for a plane
// constraint the axis perpendicular to it (XY -> Z); -1 for none or all axes.
int RotationAxis(const bool active[3]);

// The rotation for a drag of `upPixels` (up = counter-clockwise), 0.5 degrees per pixel.
math::Quat DragRotation(int axis, int upPixels);

// A node's new local rotation after turning it by `worldDelta` (in world space) about its
// own pivot, given its parent's world rotation.
math::Quat RotateInParent(const math::Quat& parentWorld, const math::Quat& worldDelta, const math::Quat& start);

// Scale factors per local axis for a drag of `upPixels` (1 % per pixel, never below
// 0.01): Uniform all three; NonUniform the constrained axes; Squash the constrained axes,
// the others shrinking so the volume stays the same.
enum class ScaleMode { Uniform, NonUniform, Squash };
math::Vec3 DragScale(ScaleMode mode, const bool active[3], int upPixels);

} // namespace scene::move
