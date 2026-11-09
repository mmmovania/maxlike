#include "scene/Modifier.h"
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace scene {

EditMeshModifier::EditMeshModifier(size_t expectedVertexCount, std::vector<Move> moves)
    : expected_(expectedVertexCount), moves_(std::move(moves)) {
    params_.Define(kIgnoreBackfacing, ParamType::Bool, 0);
    params_.Define(kPlanarThresh, ParamType::Float, 45);
    params_.Define(kExtrudeAmount, ParamType::Float, 0);
    params_.Define(kChamferAmount, ParamType::Float, 0);
    params_.Define(kSplit, ParamType::Bool, 0);
    params_.Define(kRefineEnds, ParamType::Bool, 1);
    params_.Define(kWeldThreshold, ParamType::Float, 0.1);
    params_.Define(kNormalLocal, ParamType::Int, 0);
    params_.Define(kWeldTargetPixels, ParamType::Int, 4);
    params_.Define(kTessTension, ParamType::Float, 25);
    params_.Define(kTessByFace, ParamType::Int, 0);
    params_.Define(kExplodeAngle, ParamType::Float, 24);
    params_.Define(kExplodeToElements, ParamType::Int, 0);
}

bool EditMeshModifier::Apply(Mesh& mesh) const {
    if (mesh.verts.size() != expected_) return false;   // indices would point elsewhere
    mesh = Edited(mesh);
    return true;
}

namespace {
std::vector<uint32_t> SortedUnique(std::vector<uint32_t> v) {
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    return v;
}
bool Contains(const std::vector<uint32_t>& sorted, uint32_t x) { return std::binary_search(sorted.begin(), sorted.end(), x); }
std::vector<uint32_t> Without(const std::vector<uint32_t>& v, const std::vector<uint32_t>& removedSorted) {
    std::vector<uint32_t> out;
    for (uint32_t x : v) if (!Contains(removedSorted, x)) out.push_back(x);
    return out;
}
} // namespace

void EditMeshModifier::SetSelectedFaces(std::vector<uint32_t> f) { selectedFaces_ = SortedUnique(std::move(f)); }

void EditMeshModifier::SetDeleted(std::vector<uint32_t> faces, std::vector<uint32_t> vertices) {
    deletedFaces_ = SortedUnique(std::move(faces));
    deletedVerts_ = SortedUnique(std::move(vertices));
}

void EditMeshModifier::SetState(const State& s) {
    moves_ = s.moves;
    SetSelectedVertices(s.selectedVertices);
    SetSelectedFaces(s.selectedFaces);
    SetDeleted(s.deletedFaces, s.deletedVertices);
}

Mesh EditMeshModifier::Edited(const Mesh& input, std::vector<uint32_t>* vertexMap, std::vector<uint32_t>* faceMap) const {
    Mesh m = input;
    for (const Move& mv : moves_)
        if (mv.vertex < m.verts.size()) m.verts[mv.vertex] = m.verts[mv.vertex] + mv.delta;
    if (vertexMap) vertexMap->clear();
    if (faceMap) faceMap->clear();
    if (deletedFaces_.empty() && deletedVerts_.empty()) {
        if (vertexMap) for (uint32_t i = 0; i < m.verts.size(); ++i) vertexMap->push_back(i);
        if (faceMap) for (uint32_t i = 0; i < m.faces.size(); ++i) faceMap->push_back(i);
        return m;
    }
    std::vector<uint32_t> newIndex(m.verts.size(), UINT32_MAX);
    Mesh out;
    for (uint32_t i = 0; i < m.verts.size(); ++i)
        if (!Contains(deletedVerts_, i)) {
            newIndex[i] = static_cast<uint32_t>(out.verts.size());
            out.verts.push_back(m.verts[i]);
            if (vertexMap) vertexMap->push_back(i);
        }
    for (uint32_t f = 0; f < m.faces.size(); ++f) {
        if (Contains(deletedFaces_, f)) continue;
        Face face = m.faces[f];
        bool keep = true;
        for (uint32_t& v : face.v) {
            if (v >= newIndex.size() || newIndex[v] == UINT32_MAX) { keep = false; break; }
            v = newIndex[v];
        }
        if (!keep) continue;                                  // a face goes with its vertices
        out.faces.push_back(face);
        if (faceMap) faceMap->push_back(f);
    }
    return out;
}

EditMeshModifier::State EditMeshModifier::AfterDeletingFaces(const Mesh& input, bool isolated) const {
    State s = GetState();
    std::vector<uint32_t> faces = s.deletedFaces;
    faces.insert(faces.end(), selectedFaces_.begin(), selectedFaces_.end());
    s.deletedFaces = SortedUnique(faces);
    if (isolated) {
        // Vertices no remaining face uses.
        std::vector<char> used(input.verts.size(), 0);
        for (uint32_t f = 0; f < input.faces.size(); ++f) {
            if (Contains(s.deletedFaces, f)) continue;
            bool gone = false;
            for (uint32_t v : input.faces[f].v) gone = gone || Contains(s.deletedVertices, v);
            if (gone) continue;
            for (uint32_t v : input.faces[f].v) if (v < used.size()) used[v] = 1;
        }
        for (uint32_t v = 0; v < used.size(); ++v) if (!used[v]) s.deletedVertices.push_back(v);
        s.deletedVertices = SortedUnique(s.deletedVertices);
    }
    s.selectedFaces.clear();
    s.selectedVertices = Without(s.selectedVertices, s.deletedVertices);
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterDeletingVertices() const {
    State s = GetState();
    std::vector<uint32_t> verts = s.deletedVertices;
    verts.insert(verts.end(), selected_.begin(), selected_.end());
    s.deletedVertices = SortedUnique(verts);
    s.selectedVertices.clear();
    return s;
}

Mesh DerivedObject::Evaluate() const { return EvaluateUpTo(modifiers_.size()); }

Mesh DerivedObject::EvaluateUpTo(size_t count) const {
    skipped_.clear();
    Mesh mesh = base_ ? base_->Evaluate() : Mesh{};
    for (size_t i = 0; i < modifiers_.size() && i < count; ++i) {
        const auto& mod = modifiers_[i];
        if (!mod || !mod->Enabled()) continue;
        if (!mod->Apply(mesh)) skipped_.push_back(mod->ClassName());
    }
    return mesh;
}

void EditMeshModifier::SetSelectedVertices(std::vector<uint32_t> v) {
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    selected_ = std::move(v);
}

std::vector<EditMeshModifier::Move> EditMeshModifier::AddToMoves(const std::vector<Move>& moves,
                                                                 const std::vector<uint32_t>& vertices,
                                                                 const math::Vec3& delta) {
    std::vector<Move> out = moves;
    for (uint32_t v : vertices) {
        bool found = false;
        for (Move& m : out)
            if (m.vertex == v) { m.delta = m.delta + delta; found = true; break; }
        if (!found) out.push_back({v, delta});
    }
    return out;
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
