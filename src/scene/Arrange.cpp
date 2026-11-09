#include "scene/Arrange.h"
#include <cmath>
#include <algorithm>
#include "scene/Groups.h"
#include "scene/TypeIn.h"

namespace scene::tools {

math::Mat4 FrameScale(const move::Frame& f, const double s[3]) {
    // M = sum_a s[a] * axis_a axis_a^T (column vectors).
    math::Mat4 m = math::Translation({0, 0, 0});
    for (int c = 0; c < 3; ++c)
        for (int r = 0; r < 3; ++r) {
            double v = 0;
            for (int a = 0; a < 3; ++a) {
                const double ac = c == 0 ? f.axis[a].x : c == 1 ? f.axis[a].y : f.axis[a].z;
                const double ar = r == 0 ? f.axis[a].x : r == 1 ? f.axis[a].y : f.axis[a].z;
                v += s[a] * ar * ac;
            }
            m.m[c][r] = v;
        }
    return m;
}

math::Mat4 MirrorMatrix(const move::Frame& f, MirrorAxis axis, const math::Vec3& centre, double offset) {
    double s[3] = {1, 1, 1};
    switch (axis) {
    case MirrorAxis::X: s[0] = -1; break;
    case MirrorAxis::Y: s[1] = -1; break;
    case MirrorAxis::Z: s[2] = -1; break;
    case MirrorAxis::XY: s[0] = s[1] = -1; break;
    case MirrorAxis::YZ: s[1] = s[2] = -1; break;
    case MirrorAxis::ZX: s[2] = s[0] = -1; break;
    }
    math::Vec3 shift{0, 0, 0};
    for (int a = 0; a < 3; ++a) if (s[a] < 0) shift = shift + f.axis[a] * offset;
    return math::Translation(centre + shift) * FrameScale(f, s) * math::Translation(centre * -1.0);
}

math::Mat4 ArrayStepMatrix(const ArrayStep& step, const move::Frame& f, const math::Vec3& centre) {
    const double s[3] = {step.scalePercent.x * 0.01, step.scalePercent.y * 0.01, step.scalePercent.z * 0.01};
    // The rotation's angles about the frame's axes, X first (as the Type-In's Euler order).
    const math::Quat q = math::Quat::FromAxisAngle(f.axis[2], step.rotateDegrees.z * 3.14159265358979323846 / 180) *
                         math::Quat::FromAxisAngle(f.axis[1], step.rotateDegrees.y * 3.14159265358979323846 / 180) *
                         math::Quat::FromAxisAngle(f.axis[0], step.rotateDegrees.x * 3.14159265358979323846 / 180);
    return math::Translation(step.move) * math::Translation(centre) * q.ToMat4() * FrameScale(f, s) * math::Translation(centre * -1.0);
}

bool WorldBounds(const Node& n, math::Vec3& lo, math::Vec3& hi) {
    if (n.IsGroupHead()) return groups::Bounds(n, lo, hi);
    if (!n.GetObject()) return false;
    lo = {1e300, 1e300, 1e300};
    hi = {-1e300, -1e300, -1e300};
    const math::Mat4 m = n.ObjectMatrix();
    const Mesh mesh = n.GetObject()->Evaluate();
    for (const auto& v : mesh.verts) {
        const math::Vec3 w = math::TransformPoint(m, v);
        lo = {std::min(lo.x, w.x), std::min(lo.y, w.y), std::min(lo.z, w.z)};
        hi = {std::max(hi.x, w.x), std::max(hi.y, w.y), std::max(hi.z, w.z)};
    }
    return !mesh.verts.empty();
}

namespace {
// The chosen point of a node, on each world axis.
math::Vec3 AlignPointOf(const Node& n, AlignPoint p) {
    const math::Vec3 pivot = math::TransformPoint(n.WorldMatrix(), {0, 0, 0});
    math::Vec3 lo, hi;
    if (p == AlignPoint::Pivot || !WorldBounds(n, lo, hi)) return pivot;
    if (p == AlignPoint::Minimum) return lo;
    if (p == AlignPoint::Maximum) return hi;
    return (lo + hi) * 0.5;
}
double Get(const math::Vec3& v, int a) { return a == 0 ? v.x : a == 1 ? v.y : v.z; }
} // namespace

PRS Aligned(Node& node, const Node& target, const AlignOptions& o) {
    const PRS before = node.Transform();
    // Orientation first (about the node's own pivot), then the position (its points
    // depend on the orientation), then the scale.
    const bool any = o.orientation[0] || o.orientation[1] || o.orientation[2];
    if (any) {
        const math::Quat mine = typein::WorldRotation(node), theirs = typein::WorldRotation(target);
        math::Quat delta;
        if (o.orientation[0] && o.orientation[1] && o.orientation[2]) {
            delta = theirs * math::Quat{-mine.x, -mine.y, -mine.z, mine.w};
        } else {
            const int a = o.orientation[0] ? 0 : o.orientation[1] ? 1 : 2;
            const math::Vec3 unit[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
            const math::Vec3 from = mine.Rotate(unit[a]), to = theirs.Rotate(unit[a]);
            const math::Vec3 axis = math::Cross(from, to);
            const double s = math::Length(axis), c = math::Dot(from, to);
            if (s > 1e-12) delta = math::Quat::FromAxisAngle(axis * (1.0 / s), std::atan2(s, c));
            else if (c < 0) {                                   // opposite: a half turn about any perpendicular
                const math::Vec3 perp = std::fabs(from.x) < 0.9 ? math::Cross(from, {1, 0, 0}) : math::Cross(from, {0, 1, 0});
                delta = math::Quat::FromAxisAngle(math::Normalize(perp), 3.14159265358979323846);
            }
        }
        const math::Vec3 p = math::TransformPoint(node.WorldMatrix(), {0, 0, 0});
        node.Transform() = Changed(node, math::Translation(p) * delta.ToMat4() * math::Translation(p * -1.0));
    }
    for (int a = 0; a < 3; ++a)
        if (o.scale[a]) {
            double& mine = a == 0 ? node.Transform().scale.x : a == 1 ? node.Transform().scale.y : node.Transform().scale.z;
            mine = Get(target.Transform().scale, a);
        }
    const math::Vec3 from = AlignPointOf(node, o.current), to = AlignPointOf(target, o.target);
    math::Vec3 shift{0, 0, 0};
    if (o.position[0]) shift.x = to.x - from.x;
    if (o.position[1]) shift.y = to.y - from.y;
    if (o.position[2]) shift.z = to.z - from.z;
    const PRS after = Changed(node, math::Translation(shift));
    node.Transform() = before;
    return after;
}

std::vector<math::Vec3> SubObjectDeltas(const std::vector<math::Vec3>& points, const math::Mat4& m, const math::Mat4& worldChange) {
    const math::Mat4 inv = math::AffineInverse(m);
    std::vector<math::Vec3> out;
    out.reserve(points.size());
    for (const math::Vec3& p : points)
        out.push_back(math::TransformPoint(inv, math::TransformPoint(worldChange, math::TransformPoint(m, p))) - p);
    return out;
}

PRS Changed(Node& node, const math::Mat4& worldChange) {
    const PRS before = node.Transform();
    Scene::SetWorldMatrix(&node, worldChange * node.WorldMatrix());
    const PRS after = node.Transform();
    node.Transform() = before;
    return after;
}

} // namespace scene::tools
