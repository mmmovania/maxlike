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

// Tools > Align (Phase 10.2): the current object's chosen point (its world bounding box's
// minimum, centre or maximum, or its pivot) moved onto the target's on each chosen world
// axis; optionally its orientation matched to the target's on chosen local axes (all
// three: the same rotation; one or two: the smallest turn bringing the first chosen axis
// onto the target's), and its scale matched on chosen axes. Groups use their members'
// bounds. NOTE: R3's exact rules for partial orientation are from memory.
enum class AlignPoint { Minimum, Center, Pivot, Maximum };
struct AlignOptions {
    bool position[3] = {false, false, false};
    AlignPoint current = AlignPoint::Center, target = AlignPoint::Center;
    bool orientation[3] = {false, false, false};
    bool scale[3] = {false, false, false};
};
// The world bounding box of a node (its own mesh, or a group's members); false if none.
bool WorldBounds(const Node& node, math::Vec3& lo, math::Vec3& hi);
PRS Aligned(Node& node, const Node& target, const AlignOptions& options);

// The linear map scaling by s[a] along the frame's axis a (Array, Mirror, sub-object Scale).
math::Mat4 FrameScale(const move::Frame& frame, const double s[3]);

// Sub-object Rotate and Scale (Phase 10.5): the object-space offsets that move `points`
// (object space, of an object placed by `objectToWorld`) by the world-space change
// `worldChange`. Done in world space, so a scaled object still turns rigidly.
std::vector<math::Vec3> SubObjectDeltas(const std::vector<math::Vec3>& points, const math::Mat4& objectToWorld,
                                        const math::Mat4& worldChange);

// A node's PRS once `worldChange` is applied to its world matrix (the node is unchanged).
PRS Changed(Node& node, const math::Mat4& worldChange);

} // namespace scene::tools
