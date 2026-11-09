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

// The status bar's X/Y/Z during a drag (NOTE: R3's exact display rules and number
// format are from memory, unverified): Move shows the movement, Rotate the angle about
// each axis in degrees, Scale the percentage per axis.
math::Vec3 RotationDegrees(int axis, int upPixels);           // the angle on the rotation axis only
math::Vec3 ScalePercent(const math::Vec3& factors);           // 1.5 -> 150

// Euler angles (degrees) of a rotation, applied X first, then Y, then Z.
math::Vec3 EulerXYZDegrees(const math::Quat& q);

// Reference coordinate systems (Phase 9.9): the axes constraints, drag planes and Rotate
// use, as three world-space unit vectors, and the system's origin (the Transform
// Coordinate Center). World is the identity.
struct Frame {
    math::Vec3 axis[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    math::Vec3 origin{0, 0, 0};
};
// Constrain in the frame: keep the components along the allowed frame axes.
math::Vec3 ConstrainInFrame(const math::Vec3& delta, const bool active[3], const Frame& f);
// DragPlaneNormal with the constraint's axes taken in the frame.
bool DragPlaneNormalInFrame(const bool active[3], const math::Vec3& viewDir, const Frame& f, math::Vec3& normal);
// DragRotation about the frame's axis `axis` (0.5 degrees per pixel).
math::Quat DragRotationInFrame(int axis, int upPixels, const Frame& f);

// The systems, as R3's toolbar lists them. View: ortho views use the screen's axes,
// perspective views the world's; Screen: the view's own right / up / out in every view;
// World and Grid (only the home grid exists): the world; Parent / Local: the parent's /
// the object's world rotation, at its pivot; Pick: not available yet (World).
enum class RefCoord { View, Screen, World, Parent, Local, Grid, Pick };
// `right` and `up`: the view's world directions; `parent` / `local`: world rotations and
// pivots of the object's parent and of the object (null: none).
Frame MakeFrame(RefCoord system, bool perspective, const math::Vec3& right, const math::Vec3& up,
                const math::Quat* parentRotation, const math::Vec3& parentPivot,
                const math::Quat* localRotation, const math::Vec3& localPivot);

} // namespace scene::move
