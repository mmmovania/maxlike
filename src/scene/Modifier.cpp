#include "scene/Modifier.h"
#include <cmath>
#include <algorithm>

namespace scene {

bool EditMeshModifier::Apply(Mesh& mesh) const {
    if (mesh.verts.size() != expected_) return false;   // indices would point elsewhere
    for (const Move& m : moves_)
        if (m.vertex < mesh.verts.size()) mesh.verts[m.vertex] = mesh.verts[m.vertex] + m.delta;
    return true;
}

Mesh DerivedObject::Evaluate() const {
    skipped_.clear();
    Mesh mesh = base_ ? base_->Evaluate() : Mesh{};
    for (const auto& mod : modifiers_) {
        if (!mod || !mod->Enabled()) continue;
        if (!mod->Apply(mesh)) skipped_.push_back(mod->ClassName());
    }
    return mesh;
}


// ---------------------------------------------------------------- Bend / Taper / Twist

namespace {

constexpr double kPi = 3.14159265358979323846;

// The coordinates of v in the modifier's frame: h along the axis, a and b the other two
// (in cyclic order, so the frame stays right-handed).
struct Frame { int h, a, b; };
Frame FrameFor(int axis) {
    axis = (axis < 0 || axis > 2) ? 2 : axis;
    return {axis, (axis + 1) % 3, (axis + 2) % 3};
}
double& At(math::Vec3& v, int i) { return i == 0 ? v.x : i == 1 ? v.y : v.z; }
double Get(const math::Vec3& v, int i) { return i == 0 ? v.x : i == 1 ? v.y : v.z; }

// The mesh's extent along an axis.
double Extent(const Mesh& mesh, int axis) {
    if (mesh.verts.empty()) return 0;
    double lo = 1e300, hi = -1e300;
    for (const auto& v : mesh.verts) { lo = std::min(lo, Get(v, axis)); hi = std::max(hi, Get(v, axis)); }
    return hi - lo;
}

void LimitDefaults(ParamBlock& p, size_t limit, size_t lower, size_t upper) {
    p.Define(limit, ParamType::Bool, 0);
    p.Define(lower, ParamType::Float, 0);
    p.Define(upper, ParamType::Float, 0);
}

} // namespace

BendModifier::BendModifier() {
    params_.Define(kAngle, ParamType::Float, 0);
    params_.Define(kDirection, ParamType::Float, 0);
    params_.Define(kAxis, ParamType::Int, 2);
    LimitDefaults(params_, kLimit, kLower, kUpper);
}

// A point at height h on the arc of radius r (bending towards +a), in the (a, h) plane;
// beyond the limits it continues along the tangent.
bool BendModifier::Apply(Mesh& mesh) const {
    const double angle = params_.Float(kAngle) * kPi / 180;
    const Frame f = FrameFor(params_.Int(kAxis, 2));
    const double L = Extent(mesh, f.h);
    if (std::fabs(angle) < 1e-12 || L <= 0) return true;
    const double r = L / angle;
    const bool limit = params_.Bool(kLimit);
    const double lo = std::min(params_.Float(kLower), 0.0), hi = std::max(params_.Float(kUpper), 0.0);
    const double dir = params_.Float(kDirection) * kPi / 180;
    const double c = std::cos(dir), s = std::sin(dir);
    for (auto& v : mesh.verts) {
        // Turn the bend plane by Direction: work in (a', b') rotated about the axis.
        const double a0 = Get(v, f.a), b0 = Get(v, f.b), h0 = Get(v, f.h);
        const double a = c * a0 + s * b0, b = -s * a0 + c * b0;
        double h = h0, extra = 0;
        if (limit) {
            if (h > hi) { extra = h - hi; h = hi; }
            else if (h < lo) { extra = h - lo; h = lo; }
        }
        const double phi = h / r;
        double na = r - (r - a) * std::cos(phi);
        double nh = (r - a) * std::sin(phi);
        na += extra * std::sin(phi);      // straight on along the tangent
        nh += extra * std::cos(phi);
        At(v, f.a) = c * na - s * b;
        At(v, f.b) = s * na + c * b;
        At(v, f.h) = nh;
    }
    return true;
}

TaperModifier::TaperModifier() {
    params_.Define(kAmount, ParamType::Float, 0);
    params_.Define(kCurve, ParamType::Float, 0);
    params_.Define(kAxis, ParamType::Int, 2);
    params_.Define(kEffect, ParamType::Int, 2);
    params_.Define(kSymmetry, ParamType::Bool, 0);
    LimitDefaults(params_, kLimit, kLower, kUpper);
}

bool TaperModifier::Apply(Mesh& mesh) const {
    const double amount = params_.Float(kAmount), curve = params_.Float(kCurve);
    const Frame f = FrameFor(params_.Int(kAxis, 2));
    const double L = Extent(mesh, f.h);
    if ((amount == 0 && curve == 0) || L <= 0) return true;
    const int effect = params_.Int(kEffect);
    const bool symmetry = params_.Bool(kSymmetry), limit = params_.Bool(kLimit);
    const double lo = std::min(params_.Float(kLower), 0.0), hi = std::max(params_.Float(kUpper), 0.0);
    for (auto& v : mesh.verts) {
        double h = Get(v, f.h);
        if (limit) h = std::clamp(h, lo, hi);
        double t = h / L;
        if (symmetry) t = std::fabs(t);
        const double scale = 1 + amount * t + curve * 4 * t * (1 - t);
        if (effect != 1) At(v, f.a) *= scale;
        if (effect != 0) At(v, f.b) *= scale;
    }
    return true;
}

TwistModifier::TwistModifier() {
    params_.Define(kAngle, ParamType::Float, 0);
    params_.Define(kBias, ParamType::Float, 0);
    params_.Define(kAxis, ParamType::Int, 2);
    LimitDefaults(params_, kLimit, kLower, kUpper);
}

bool TwistModifier::Apply(Mesh& mesh) const {
    const double angle = params_.Float(kAngle) * kPi / 180;
    const Frame f = FrameFor(params_.Int(kAxis, 2));
    const double L = Extent(mesh, f.h);
    if (std::fabs(angle) < 1e-12 || L <= 0) return true;
    const double bias = std::clamp(params_.Float(kBias), -100.0, 100.0);
    const bool limit = params_.Bool(kLimit);
    const double lo = std::min(params_.Float(kLower), 0.0), hi = std::max(params_.Float(kUpper), 0.0);
    for (auto& v : mesh.verts) {
        double h = Get(v, f.h);
        if (limit) h = std::clamp(h, lo, hi);
        double t = h / L;
        // Bias pushes the twist towards one end: t^k with k = 2^(-bias/25).
        if (bias != 0) t = (t < 0 ? -1 : 1) * std::pow(std::fabs(t), std::pow(2.0, -bias / 25));
        const double psi = angle * t, c = std::cos(psi), s = std::sin(psi);
        const double a = Get(v, f.a), b = Get(v, f.b);
        At(v, f.a) = c * a - s * b;
        At(v, f.b) = s * a + c * b;
    }
    return true;
}

} // namespace scene
