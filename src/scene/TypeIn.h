#pragma once
#include "scene/Scene.h"

// Tools > Transform Type-In (Phase 9.8): exact values for Move, Rotate and Scale.
// Absolute values are in world space (Scale: the object's own, in percent); offsets are
// added in world space (Rotate: degrees about the world axes, around a pivot; Scale: a
// percentage of the current scale, 100 = unchanged). Nothing here changes the node: the
// results go into a TransformCommand.
namespace scene::typein {

enum class Tool { Move, Rotate, Scale };

// The inverse of scene::move::EulerXYZDegrees (X first, then Y, then Z).
math::Quat QuatFromEulerXYZDegrees(const math::Vec3& degrees);
// The node's rotation in the world: its parents' rotations, then its own.
math::Quat WorldRotation(const Node& node);

math::Vec3 Absolute(Tool tool, const Node& node);
PRS WithAbsolute(Tool tool, Node& node, const math::Vec3& value);
PRS WithOffset(Tool tool, Node& node, const math::Vec3& offset, const math::Vec3& pivot);

} // namespace scene::typein
