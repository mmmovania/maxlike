#include "scene/TypeIn.h"
#include "scene/MoveMath.h"

namespace scene::typein {

namespace {
const double kRad = 3.14159265358979323846 / 180.0;

math::Vec3 WorldPosition(const Node& n) { return math::TransformPoint(n.WorldMatrix(), {0, 0, 0}); }

// The node's PRS once its world matrix becomes `world` (solved by the scene's own rule),
// leaving the node as it was.
PRS Solved(Node& node, const math::Mat4& world) {
    const PRS before = node.Transform();
    Scene::SetWorldMatrix(&node, world);
    const PRS after = node.Transform();
    node.Transform() = before;
    return after;
}

// A world rotation `delta` about the point `p`, applied after the node's world matrix.
PRS RotatedAbout(Node& node, const math::Quat& delta, const math::Vec3& p) {
    return Solved(node, math::Translation(p) * delta.ToMat4() * math::Translation(p * -1.0) * node.WorldMatrix());
}
} // namespace

math::Quat QuatFromEulerXYZDegrees(const math::Vec3& d) {
    return math::Quat::FromAxisAngle({0, 0, 1}, d.z * kRad) * math::Quat::FromAxisAngle({0, 1, 0}, d.y * kRad) *
           math::Quat::FromAxisAngle({1, 0, 0}, d.x * kRad);
}

math::Quat WorldRotation(const Node& node) {
    math::Quat q = node.Transform().rotation;
    for (const Node* p = node.Parent(); p; p = p->Parent()) q = p->Transform().rotation * q;
    return q.Normalized();
}

math::Vec3 Absolute(Tool tool, const Node& node) {
    switch (tool) {
    case Tool::Move: return WorldPosition(node);
    case Tool::Rotate: return move::EulerXYZDegrees(WorldRotation(node));
    case Tool::Scale: return node.Transform().scale * 100.0;
    }
    return {0, 0, 0};
}

PRS WithAbsolute(Tool tool, Node& node, const math::Vec3& v) {
    switch (tool) {
    case Tool::Move: return Solved(node, math::Translation(v - WorldPosition(node)) * node.WorldMatrix());
    case Tool::Rotate: {
        // From the current world rotation to the typed one, about the node's own pivot.
        const math::Quat w = WorldRotation(node);
        const math::Quat delta = QuatFromEulerXYZDegrees(v) * math::Quat{-w.x, -w.y, -w.z, w.w};   // the conjugate: w's inverse
        return RotatedAbout(node, delta, WorldPosition(node));
    }
    case Tool::Scale: {
        PRS t = node.Transform();
        t.scale = v * 0.01;
        return t;
    }
    }
    return node.Transform();
}

PRS WithOffset(Tool tool, Node& node, const math::Vec3& o, const math::Vec3& pivot) {
    switch (tool) {
    case Tool::Move: return Solved(node, math::Translation(o) * node.WorldMatrix());
    case Tool::Rotate: return RotatedAbout(node, QuatFromEulerXYZDegrees(o), pivot);
    case Tool::Scale: {
        // NOTE: each object scales about its own pivot (R3 may scale a selection about a
        // common centre; unverified).
        PRS t = node.Transform();
        t.scale = {t.scale.x * o.x * 0.01, t.scale.y * o.y * 0.01, t.scale.z * o.z * 0.01};
        return t;
    }
    }
    return node.Transform();
}

PRS WithOffset(Tool tool, Node& node, const math::Vec3& o, const math::Vec3& pivot, const move::Frame& f) {
    if (tool == Tool::Move)
        return WithOffset(tool, node, f.axis[0] * o.x + f.axis[1] * o.y + f.axis[2] * o.z, pivot);
    if (tool == Tool::Rotate)
        return RotatedAbout(node, math::Quat::FromAxisAngle(f.axis[2], o.z * kRad) * math::Quat::FromAxisAngle(f.axis[1], o.y * kRad) *
                                      math::Quat::FromAxisAngle(f.axis[0], o.x * kRad), pivot);
    return WithOffset(tool, node, o, pivot);
}

} // namespace scene::typein
