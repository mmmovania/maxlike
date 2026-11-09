#pragma once
#include "scene/MoveMath.h"
#include "scene/Scene.h"

// Tools > Mirror and Tools > Array (Phase 9.10). Matrices act on world space; the results
// go through Scene::SetWorldMatrix, which keeps a mirror as a negative X scale.
namespace scene::tools {

// Mirror across the reference frame's axis (X: X flipped) or both axes of a plane option
// (XY: X and Y flipped), through `centre`, then shifted by `offset` along each flipped axis.
// NOTE: R3's semantics for the plane options and for Offset are from memory.
enum class MirrorAxis { X, Y, Z, XY, YZ, ZX };
math::Mat4 MirrorMatrix(const move::Frame& frame, MirrorAxis axis, const math::Vec3& centre, double offset);

// One step of an array: a world move, a rotation (degrees about the frame's axes) and a
// scale (percent, per frame axis), the last two about `centre`.
struct ArrayStep {
    math::Vec3 move{0, 0, 0};
    math::Vec3 rotateDegrees{0, 0, 0};
    math::Vec3 scalePercent{100, 100, 100};
};
math::Mat4 ArrayStepMatrix(const ArrayStep& step, const move::Frame& frame, const math::Vec3& centre);

// A node's PRS once `worldChange` is applied to its world matrix (the node is unchanged).
PRS Changed(Node& node, const math::Mat4& worldChange);

} // namespace scene::tools
