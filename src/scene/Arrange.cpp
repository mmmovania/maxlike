#include "scene/Arrange.h"
#include "scene/TypeIn.h"

namespace scene::tools {

namespace {
// The 3 x 3 linear map that scales by s[a] along the frame's axis a.
math::Mat4 AlongFrame(const move::Frame& f, const double s[3]) {
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
} // namespace

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
    return math::Translation(centre + shift) * AlongFrame(f, s) * math::Translation(centre * -1.0);
}

math::Mat4 ArrayStepMatrix(const ArrayStep& step, const move::Frame& f, const math::Vec3& centre) {
    const double s[3] = {step.scalePercent.x * 0.01, step.scalePercent.y * 0.01, step.scalePercent.z * 0.01};
    // The rotation's angles about the frame's axes, X first (as the Type-In's Euler order).
    const math::Quat q = math::Quat::FromAxisAngle(f.axis[2], step.rotateDegrees.z * 3.14159265358979323846 / 180) *
                         math::Quat::FromAxisAngle(f.axis[1], step.rotateDegrees.y * 3.14159265358979323846 / 180) *
                         math::Quat::FromAxisAngle(f.axis[0], step.rotateDegrees.x * 3.14159265358979323846 / 180);
    return math::Translation(step.move) * math::Translation(centre) * q.ToMat4() * AlongFrame(f, s) * math::Translation(centre * -1.0);
}

PRS Changed(Node& node, const math::Mat4& worldChange) {
    const PRS before = node.Transform();
    Scene::SetWorldMatrix(&node, worldChange * node.WorldMatrix());
    const PRS after = node.Transform();
    node.Transform() = before;
    return after;
}

} // namespace scene::tools
