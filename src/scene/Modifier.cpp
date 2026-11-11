#include "scene/Modifier.h"
#include "scene/MeshTopology.h"
#include <cmath>
#include <algorithm>
#include <map>
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
    params_.Define(kUseSoftSelection, ParamType::Bool, 0);
    params_.Define(kEdgeDistance, ParamType::Bool, 0);
    params_.Define(kEdgeIterations, ParamType::Int, 1);
    params_.Define(kAffectBackfacing, ParamType::Bool, 1);
    params_.Define(kFalloff, ParamType::Float, 20);
    params_.Define(kPinch, ParamType::Float, 0);
    params_.Define(kBubble, ParamType::Float, 0);
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
void EditMeshModifier::SetSelectedEdges(std::vector<uint32_t> e) { selectedEdges_ = SortedUnique(std::move(e)); }

void EditMeshModifier::SetDeleted(std::vector<uint32_t> faces, std::vector<uint32_t> vertices) {
    deletedFaces_ = SortedUnique(std::move(faces));
    deletedVerts_ = SortedUnique(std::move(vertices));
}

void EditMeshModifier::SetState(const State& s) {
    moves_ = s.moves;
    SetSelectedVertices(s.selectedVertices);
    SetSelectedFaces(s.selectedFaces);
    SetSelectedEdges(s.selectedEdges);
    SetDeleted(s.deletedFaces, s.deletedVertices);
    hiddenFaces_ = SortedUnique(s.hiddenFaces);
    hiddenVerts_ = SortedUnique(s.hiddenVertices);
    welds_ = s.welds;
    addedVertices_ = s.addedVertices;
    addedFaces_ = s.addedFaces;
}

Mesh EditMeshModifier::Edited(const Mesh& input, std::vector<uint32_t>* vertexMap, std::vector<uint32_t>* faceMap) const {
    Mesh m = Extended(input);                                // added geometry first (10.7)
    for (const Move& mv : moves_)
        if (mv.vertex < m.verts.size()) m.verts[mv.vertex] = m.verts[mv.vertex] + mv.delta;
    if (vertexMap) vertexMap->clear();
    if (faceMap) faceMap->clear();
    if (deletedFaces_.empty() && deletedVerts_.empty() && welds_.empty() && hiddenFaces_.empty()) {   // (added faces need no remapping)
        if (vertexMap) for (uint32_t i = 0; i < m.verts.size(); ++i) vertexMap->push_back(i);
        if (faceMap) for (uint32_t i = 0; i < m.faces.size(); ++i) faceMap->push_back(i);
        return m;
    }
    // Welds: each merged vertex follows its chain to the vertex that remains.
    std::vector<uint32_t> target(m.verts.size());
    for (uint32_t i = 0; i < target.size(); ++i) target[i] = i;
    for (const auto& [from, to] : welds_)
        if (from < target.size() && to < target.size()) target[from] = to;
    for (uint32_t i = 0; i < target.size(); ++i)
        for (int guard = 0; target[target[i]] != target[i] && guard < 64; ++guard) target[i] = target[target[i]];
    std::vector<uint32_t> newIndex(m.verts.size(), UINT32_MAX);
    Mesh out;
    for (uint32_t i = 0; i < m.verts.size(); ++i)
        if (!Contains(deletedVerts_, i) && target[i] == i) {
            newIndex[i] = static_cast<uint32_t>(out.verts.size());
            out.verts.push_back(m.verts[i]);
            if (vertexMap) vertexMap->push_back(i);
        }
    for (uint32_t f = 0; f < m.faces.size(); ++f) {
        if (Contains(deletedFaces_, f) || Contains(hiddenFaces_, f)) continue;
        Face face = m.faces[f];
        bool keep = true;
        for (uint32_t& v : face.v) {
            if (v >= target.size()) { keep = false; break; }
            v = target[v];
            if (newIndex[v] == UINT32_MAX) { keep = false; break; }
            v = newIndex[v];
        }
        if (!keep) continue;                                  // a face goes with its vertices
        if (face.v[0] == face.v[1] || face.v[1] == face.v[2] || face.v[2] == face.v[0]) continue;   // collapsed by a weld
        out.faces.push_back(face);
        if (faceMap) faceMap->push_back(f);
    }
    return out;
}

EditMeshModifier::State EditMeshModifier::AfterDeletingFaces(const Mesh& raw, bool isolated) const {
    const Mesh input = Extended(raw);                       // added geometry too (10.7)
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

EditMeshModifier::State EditMeshModifier::AfterDeletingEdges(const Mesh& input, bool isolated) const {
    // The faces on both sides of each selected edge, deleted as selected faces would be.
    EditMeshModifier copy = *this;
    std::vector<uint32_t> faces;
    for (uint32_t e : selectedEdges_)
        for (uint32_t f : FacesOnEdge(Extended(input), e)) faces.push_back(f);
    copy.SetSelectedFaces(faces);
    State s = copy.AfterDeletingFaces(input, isolated);
    s.selectedFaces = selectedFaces_;
    s.selectedFaces = Without(s.selectedFaces, s.deletedFaces);
    s.selectedEdges.clear();
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterHiding(bool faces) const {
    State s = GetState();
    if (faces) {
        s.hiddenFaces.insert(s.hiddenFaces.end(), selectedFaces_.begin(), selectedFaces_.end());
        s.hiddenFaces = SortedUnique(s.hiddenFaces);
        s.selectedFaces.clear();
    } else {
        s.hiddenVertices.insert(s.hiddenVertices.end(), selected_.begin(), selected_.end());
        s.hiddenVertices = SortedUnique(s.hiddenVertices);
        s.selectedVertices.clear();
    }
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterUnhidingAll(bool faces) const {
    State s = GetState();
    (faces ? s.hiddenFaces : s.hiddenVertices).clear();
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterWelding(const Mesh& input, double threshold, size_t* merged) const {
    State s = GetState();
    if (merged) *merged = 0;
    // Positions after the moves.
    const std::vector<math::Vec3> p = Positions(input).verts;   // added vertices too (10.7)
    // Candidates: selected vertices still present (not deleted, not merged away before).
    std::vector<uint32_t> cand;
    for (uint32_t v : selected_) {
        bool gone = v >= p.size() || Contains(deletedVerts_, v);
        for (const auto& w : welds_) gone = gone || w.first == v;
        if (!gone) cand.push_back(v);
    }
    // Clusters of candidates linked by distances within the threshold (union-find).
    std::vector<size_t> parent(cand.size());
    for (size_t i = 0; i < parent.size(); ++i) parent[i] = i;
    auto find = [&](size_t i) { while (parent[i] != i) i = parent[i] = parent[parent[i]]; return i; };
    for (size_t i = 0; i < cand.size(); ++i)
        for (size_t j = i + 1; j < cand.size(); ++j)
            if (math::Length(p[cand[i]] - p[cand[j]]) <= threshold) parent[find(i)] = find(j);
    std::vector<std::vector<uint32_t>> clusters(cand.size());
    for (size_t i = 0; i < cand.size(); ++i) clusters[find(i)].push_back(cand[i]);
    std::vector<uint32_t> keep;
    for (auto& c : clusters) {
        if (c.size() < 2) { if (c.size() == 1) keep.push_back(c[0]); continue; }
        std::sort(c.begin(), c.end());
        math::Vec3 avg{0, 0, 0};
        for (uint32_t v : c) avg = avg + p[v];
        avg = avg * (1.0 / static_cast<double>(c.size()));
        const uint32_t rep = c[0];                           // the lowest index remains
        s.moves = AddToMoves(s.moves, {rep}, avg - p[rep]);
        for (size_t i = 1; i < c.size(); ++i) s.welds.push_back({c[i], rep});
        if (merged) *merged += c.size() - 1;
        keep.push_back(rep);
    }
    s.selectedVertices = SortedUnique(keep);
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterWeldingTarget(uint32_t from, uint32_t to) const {
    State s = GetState();
    if (from == to) return s;
    for (const auto& w : welds_) if (w.first == from || w.first == to) return s;   // already merged away
    if (Contains(deletedVerts_, from) || Contains(deletedVerts_, to)) return s;
    s.welds.push_back({from, to});
    s.selectedVertices = Without(s.selectedVertices, {from});
    return s;
}

Mesh EditMeshModifier::Extract(const Mesh& input, const std::vector<uint32_t>& inputFaces) const {
    std::vector<uint32_t> faceMap;
    const Mesh out = Edited(input, nullptr, &faceMap);
    const std::vector<uint32_t> wanted = SortedUnique(inputFaces);
    Mesh part;
    std::vector<uint32_t> newIndex(out.verts.size(), UINT32_MAX);
    for (size_t f = 0; f < out.faces.size(); ++f) {
        if (!Contains(wanted, faceMap[f])) continue;
        Face face = out.faces[f];
        for (uint32_t& v : face.v) {
            if (newIndex[v] == UINT32_MAX) { newIndex[v] = static_cast<uint32_t>(part.verts.size()); part.verts.push_back(out.verts[v]); }
            v = newIndex[v];
        }
        part.faces.push_back(face);
    }
    return part;
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

std::vector<EditMeshModifier::Move> EditMeshModifier::AddToMoves(const std::vector<Move>& moves, const std::vector<uint32_t>& vertices,
                                                                 const std::vector<math::Vec3>& deltas) {
    std::vector<Move> out = moves;
    for (size_t k = 0; k < vertices.size() && k < deltas.size(); ++k) out = AddToMoves(out, {vertices[k]}, deltas[k]);
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
void BendModifier::Deform(Mesh& mesh) const {
    const double angle = params_.Float(kAngle) * kPi / 180;
    const Frame f = FrameFor(params_.Int(kAxis, 2));
    const double L = Extent(mesh, f.h);
    if (std::fabs(angle) < 1e-12 || L <= 0) return;
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
    return;
}

TaperModifier::TaperModifier() {
    params_.Define(kAmount, ParamType::Float, 0);
    params_.Define(kCurve, ParamType::Float, 0);
    params_.Define(kAxis, ParamType::Int, 2);
    params_.Define(kEffect, ParamType::Int, 2);
    params_.Define(kSymmetry, ParamType::Bool, 0);
    LimitDefaults(params_, kLimit, kLower, kUpper);
}

void TaperModifier::Deform(Mesh& mesh) const {
    const double amount = params_.Float(kAmount), curve = params_.Float(kCurve);
    const Frame f = FrameFor(params_.Int(kAxis, 2));
    const double L = Extent(mesh, f.h);
    if ((amount == 0 && curve == 0) || L <= 0) return;
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
    return;
}

TwistModifier::TwistModifier() {
    params_.Define(kAngle, ParamType::Float, 0);
    params_.Define(kBias, ParamType::Float, 0);
    params_.Define(kAxis, ParamType::Int, 2);
    LimitDefaults(params_, kLimit, kLower, kUpper);
}

void TwistModifier::Deform(Mesh& mesh) const {
    const double angle = params_.Float(kAngle) * kPi / 180;
    const Frame f = FrameFor(params_.Int(kAxis, 2));
    const double L = Extent(mesh, f.h);
    if (std::fabs(angle) < 1e-12 || L <= 0) return;
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
    return;
}

// ---- Edit Geometry (10.7) -----------------------------------------------------------------

Mesh EditMeshModifier::Extended(const Mesh& input) const {
    Mesh m = input;
    m.verts.insert(m.verts.end(), addedVertices_.begin(), addedVertices_.end());
    m.faces.insert(m.faces.end(), addedFaces_.begin(), addedFaces_.end());
    return m;
}

Mesh EditMeshModifier::Positions(const Mesh& input) const {
    Mesh m = Extended(input);
    for (const Move& mv : moves_)
        if (mv.vertex < m.verts.size()) m.verts[mv.vertex] = m.verts[mv.vertex] + mv.delta;
    return m;
}

EditMeshModifier::State EditMeshModifier::AfterRemovingIsolated(const Mesh& input) const {
    State s = GetState();
    std::vector<uint32_t> vmap, fmap;
    const Mesh out = Edited(input, &vmap, &fmap);
    std::vector<char> used(out.verts.size(), 0);
    for (const Face& f : out.faces) for (uint32_t v : f.v) if (v < used.size()) used[v] = 1;
    for (uint32_t i = 0; i < out.verts.size(); ++i) if (!used[i]) s.deletedVertices.push_back(vmap[i]);
    s.deletedVertices = SortedUnique(s.deletedVertices);
    s.selectedVertices = Without(s.selectedVertices, s.deletedVertices);
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterSelectingOpenEdges(const Mesh& input) const {
    State s = GetState();
    std::vector<uint32_t> fmap;
    const Mesh out = Edited(input, nullptr, &fmap);
    std::map<std::pair<uint32_t, uint32_t>, int> count;
    auto key = [](uint32_t a, uint32_t b) { return a < b ? std::make_pair(a, b) : std::make_pair(b, a); };
    for (const Face& f : out.faces) for (int k = 0; k < 3; ++k) ++count[key(f.v[k], f.v[(k + 1) % 3])];
    s.selectedEdges.clear();
    for (uint32_t f = 0; f < out.faces.size(); ++f)
        for (int k = 0; k < 3; ++k)
            if (count[key(out.faces[f].v[k], out.faces[f].v[(k + 1) % 3])] == 1) s.selectedEdges.push_back(fmap[f] * 3 + static_cast<uint32_t>(k));
    s.selectedEdges = SortedUnique(s.selectedEdges);
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterCollapsing(const Mesh& input, const std::vector<uint32_t>& vertices) const {
    // Weld Selected with no distance limit, on these vertices.
    EditMeshModifier copy = *this;
    copy.SetSelectedVertices(vertices);
    State s = copy.AfterWelding(input, 1e300, nullptr);
    s.selectedVertices = selected_;
    s.selectedVertices = Without(s.selectedVertices, [&] { std::vector<uint32_t> gone; for (const auto& w : s.welds) gone.push_back(w.first); return SortedUnique(gone); }());
    return s;
}

namespace {
// The moves projecting `vertices` (at positions `p`) onto the plane through `q` with unit `n`.
std::vector<EditMeshModifier::Move> Projected(const std::vector<EditMeshModifier::Move>& moves, const Mesh& p,
                                              const std::vector<uint32_t>& vertices, const math::Vec3& q, const math::Vec3& n) {
    std::vector<uint32_t> vs;
    std::vector<math::Vec3> ds;
    for (uint32_t v : vertices) {
        if (v >= p.verts.size()) continue;
        vs.push_back(v);
        ds.push_back(n * -math::Dot(p.verts[v] - q, n));
    }
    return EditMeshModifier::AddToMoves(moves, vs, ds);
}
math::Vec3 Centroid(const Mesh& p, const std::vector<uint32_t>& vertices) {
    math::Vec3 c{0, 0, 0};
    size_t n = 0;
    for (uint32_t v : vertices) if (v < p.verts.size()) { c = c + p.verts[v]; ++n; }
    return n ? c * (1.0 / static_cast<double>(n)) : c;
}
} // namespace

EditMeshModifier::State EditMeshModifier::AfterMakingPlanar(const Mesh& input, const std::vector<uint32_t>& vertices) const {
    State s = GetState();
    const Mesh p = Positions(input);
    // The normal: the (area-weighted) normals of the faces touching the vertices.
    math::Vec3 n{0, 0, 0};
    for (const Face& f : p.faces) {
        bool touches = false;
        for (uint32_t v : f.v) touches = touches || std::binary_search(vertices.begin(), vertices.end(), v);
        if (touches && f.v[0] < p.verts.size() && f.v[1] < p.verts.size() && f.v[2] < p.verts.size())
            n = n + math::Cross(p.verts[f.v[1]] - p.verts[f.v[0]], p.verts[f.v[2]] - p.verts[f.v[0]]);
    }
    if (math::Length(n) < 1e-12) n = {0, 0, 1};
    s.moves = Projected(s.moves, p, vertices, Centroid(p, vertices), math::Normalize(n));
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterAligning(const Mesh& input, const std::vector<uint32_t>& vertices,
                                                        const math::Vec3& normal, const math::Vec3* point) const {
    State s = GetState();
    const Mesh p = Positions(input);
    s.moves = Projected(s.moves, p, vertices, point ? *point : Centroid(p, vertices), math::Normalize(normal));
    return s;
}

// ---- Edit Geometry, topology (10.8) ----------------------------------------------------------

namespace {
// Live faces of the extended mesh: not deleted, not hidden, none of their vertices deleted
// or welded away.
std::vector<char> LiveFaces(const Mesh& ext, const EditMeshModifier::State& s) {
    std::vector<char> gone(ext.verts.size(), 0);
    for (uint32_t v : s.deletedVertices) if (v < gone.size()) gone[v] = 1;
    for (const auto& w : s.welds) if (w.first < gone.size()) gone[w.first] = 1;
    std::vector<char> live(ext.faces.size(), 1);
    for (uint32_t f = 0; f < ext.faces.size(); ++f) {
        if (std::binary_search(s.deletedFaces.begin(), s.deletedFaces.end(), f) ||
            std::binary_search(s.hiddenFaces.begin(), s.hiddenFaces.end(), f)) { live[f] = 0; continue; }
        for (uint32_t v : ext.faces[f].v) if (v >= gone.size() || gone[v]) live[f] = 0;
    }
    return live;
}
// Appends a vertex (its final position: added vertices carry no move) and returns its index.
uint32_t AddVertex(EditMeshModifier::State& s, size_t inputCount, const math::Vec3& p) {
    s.addedVertices.push_back(p);
    return static_cast<uint32_t>(inputCount + s.addedVertices.size() - 1);
}
// Replaces face f by `faces` (deleted, then added); returns the new faces' indices.
std::vector<uint32_t> ReplaceFace(EditMeshModifier::State& s, size_t inputFaces, uint32_t f, const std::vector<Face>& faces) {
    s.deletedFaces.push_back(f);
    std::vector<uint32_t> out;
    for (const Face& nf : faces) {
        s.addedFaces.push_back(nf);
        out.push_back(static_cast<uint32_t>(inputFaces + s.addedFaces.size() - 1));
    }
    return out;
}
Face MakeFace(const Face& like, uint32_t a, uint32_t b, uint32_t c, uint32_t visible) {
    Face f = like;
    f.v[0] = a; f.v[1] = b; f.v[2] = c;
    f.edgeVisible = visible;
    return f;
}
math::Vec3 FaceNormal(const Mesh& p, const Face& f) {
    return math::Cross(p.verts[f.v[1]] - p.verts[f.v[0]], p.verts[f.v[2]] - p.verts[f.v[0]]);
}
using EdgeKey = std::pair<uint32_t, uint32_t>;
EdgeKey Key(uint32_t a, uint32_t b) { return a < b ? EdgeKey{a, b} : EdgeKey{b, a}; }
} // namespace

EditMeshModifier::State EditMeshModifier::AfterBreaking(const Mesh& input) const {
    State s = GetState();
    const Mesh p = Positions(input), ext = Extended(input);
    const std::vector<char> live = LiveFaces(ext, s);
    const size_t inV = input.verts.size(), inF = input.faces.size();
    std::vector<uint32_t> newSelection = s.selectedVertices;
    std::map<uint32_t, Face> replaced;                         // face -> its new corners
    for (uint32_t v : selected_) {
        bool first = true;
        for (uint32_t f = 0; f < ext.faces.size(); ++f) {
            if (!live[f]) continue;
            Face face = replaced.count(f) ? replaced[f] : ext.faces[f];
            for (uint32_t& c : face.v) {
                if (c != v) continue;
                if (first) { first = false; continue; }
                c = AddVertex(s, inV, p.verts[v]);
                newSelection.push_back(c);
                replaced[f] = face;
            }
        }
    }
    for (const auto& [f, face] : replaced) ReplaceFace(s, inF, f, {face});
    s.deletedFaces = SortedUnique(s.deletedFaces);
    s.selectedVertices = SortedUnique(newSelection);
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterTurning(const Mesh& input) const {
    State s = GetState();
    const Mesh ext = Extended(input);
    const std::vector<char> live = LiveFaces(ext, s);
    const size_t inF = input.faces.size();
    std::vector<char> used(ext.faces.size(), 0);
    for (uint32_t e : selectedEdges_) {
        const uint32_t f1 = e / 3;
        if (f1 >= ext.faces.size() || !live[f1] || used[f1]) continue;
        const int k1 = static_cast<int>(e % 3);
        const Face& A = ext.faces[f1];
        const uint32_t a = A.v[k1], b = A.v[(k1 + 1) % 3], c = A.v[(k1 + 2) % 3];
        // The other face with the edge b -> a.
        for (uint32_t f2 = 0; f2 < ext.faces.size(); ++f2) {
            if (f2 == f1 || !live[f2] || used[f2]) continue;
            const Face& B = ext.faces[f2];
            for (int k2 = 0; k2 < 3; ++k2) {
                if (B.v[k2] != b || B.v[(k2 + 1) % 3] != a) continue;
                const uint32_t d = B.v[(k2 + 2) % 3];
                // The quad a, d, b, c (counter-clockwise); new diagonal c - d. Outer edges keep
                // their visibility: a-d and d-b from B, b-c and c-a from A.
                auto vis = [](const Face& f, int side) { return (f.edgeVisible >> side) & 1u; };
                const uint32_t ad = vis(B, (k2 + 1) % 3), db = vis(B, (k2 + 2) % 3);
                const uint32_t bc = vis(A, (k1 + 1) % 3), ca = vis(A, (k1 + 2) % 3);
                ReplaceFace(s, inF, f1, {MakeFace(A, a, d, c, ad | (0u << 1) | (ca << 2))});
                ReplaceFace(s, inF, f2, {MakeFace(A, d, b, c, db | (bc << 1) | (0u << 2))});
                used[f1] = used[f2] = 1;
                break;
            }
            if (used[f1]) break;
        }
    }
    s.deletedFaces = SortedUnique(s.deletedFaces);
    s.selectedEdges.clear();
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterTessellating(const Mesh& input, bool byCenter) const {
    State s = GetState();
    const Mesh p = Positions(input), ext = Extended(input);
    const std::vector<char> live = LiveFaces(ext, s);
    const size_t inV = input.verts.size(), inF = input.faces.size();
    std::map<EdgeKey, uint32_t> mid;
    std::vector<uint32_t> newFaces;
    for (uint32_t f : selectedFaces_) {
        if (f >= ext.faces.size() || !live[f]) continue;
        const Face& F = ext.faces[f];
        std::vector<uint32_t> made;
        if (byCenter) {
            const uint32_t c = AddVertex(s, inV, (p.verts[F.v[0]] + p.verts[F.v[1]] + p.verts[F.v[2]]) * (1.0 / 3.0));
            std::vector<Face> parts;
            for (int k = 0; k < 3; ++k)
                parts.push_back(MakeFace(F, F.v[k], F.v[(k + 1) % 3], c, ((F.edgeVisible >> k) & 1u) | 2u | 4u));
            made = ReplaceFace(s, inF, f, parts);
        } else {
            uint32_t m[3];
            for (int k = 0; k < 3; ++k) {
                const EdgeKey key = Key(F.v[k], F.v[(k + 1) % 3]);
                auto it = mid.find(key);
                m[k] = it != mid.end() ? it->second : (mid[key] = AddVertex(s, inV, (p.verts[F.v[k]] + p.verts[F.v[(k + 1) % 3]]) * 0.5));
            }
            auto ev = [&](int k) { return (F.edgeVisible >> k) & 1u; };
            made = ReplaceFace(s, inF, f, {MakeFace(F, F.v[0], m[0], m[2], ev(0) | 2u | (ev(2) << 2)),
                                           MakeFace(F, m[0], F.v[1], m[1], ev(0) | (ev(1) << 1) | 4u),
                                           MakeFace(F, m[2], m[1], F.v[2], 1u | (ev(1) << 1) | (ev(2) << 2)),
                                           MakeFace(F, m[0], m[1], m[2], 7u)});
        }
        newFaces.insert(newFaces.end(), made.begin(), made.end());
    }
    s.deletedFaces = SortedUnique(s.deletedFaces);
    s.selectedFaces = SortedUnique(newFaces);
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterExtruding(const Mesh& input, double amount) const {
    State s = GetState();
    const Mesh p = Positions(input), ext = Extended(input);
    const std::vector<char> live = LiveFaces(ext, s);
    const size_t inV = input.verts.size(), inF = input.faces.size();
    std::vector<uint32_t> faces;
    for (uint32_t f : selectedFaces_) if (f < ext.faces.size() && live[f]) faces.push_back(f);
    if (faces.empty()) return s;
    // The direction: the selection's averaged (area-weighted) normal, as R3's Group normal.
    math::Vec3 n{0, 0, 0};
    for (uint32_t f : faces) n = n + FaceNormal(p, ext.faces[f]);
    if (math::Length(n) < 1e-12) return s;
    n = math::Normalize(n);
    // Each vertex of the selection gets a copy moved by the amount.
    std::map<uint32_t, uint32_t> copy;
    for (uint32_t f : faces)
        for (uint32_t v : ext.faces[f].v)
            if (!copy.count(v)) copy[v] = AddVertex(s, inV, p.verts[v] + n * amount);
    // Boundary edges (directed as in their selected face): sides only one selected face has.
    std::map<EdgeKey, int> count;
    for (uint32_t f : faces) for (int k = 0; k < 3; ++k) ++count[Key(ext.faces[f].v[k], ext.faces[f].v[(k + 1) % 3])];
    std::vector<uint32_t> moved;
    for (uint32_t f : faces) {
        const Face& F = ext.faces[f];
        for (int k = 0; k < 3; ++k) {
            const uint32_t a = F.v[k], b = F.v[(k + 1) % 3];
            if (count[Key(a, b)] != 1) continue;
            // The wall a, b, b', a' faces outwards (normal (b - a) x n).
            s.addedFaces.push_back(MakeFace(F, a, b, copy[b], 1u | 2u));
            s.addedFaces.push_back(MakeFace(F, a, copy[b], copy[a], 2u | 4u));
        }
        const auto made = ReplaceFace(s, inF, f, {MakeFace(F, copy[F.v[0]], copy[F.v[1]], copy[F.v[2]], F.edgeVisible)});
        moved.insert(moved.end(), made.begin(), made.end());
    }
    s.deletedFaces = SortedUnique(s.deletedFaces);
    s.selectedFaces = SortedUnique(moved);
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterChamfering(const Mesh& input, double amount) const {
    State s = GetState();
    const size_t inV = input.verts.size(), inF = input.faces.size();
    if (amount <= 0) return s;
    for (uint32_t v : selected_) {
        // The mesh as it is now, after the vertices chamfered before this one.
        EditMeshModifier now = *this;
        now.SetState(s);
        const Mesh p = now.Positions(input), ext = now.Extended(input);
        const std::vector<char> live = LiveFaces(ext, s);
        if (v >= p.verts.size()) continue;
        std::map<uint32_t, uint32_t> along;                    // neighbour -> the new vertex on that edge
        auto onEdge = [&](uint32_t q) {
            auto it = along.find(q);
            if (it != along.end()) return it->second;
            const math::Vec3 d = p.verts[q] - p.verts[v];
            const double len = math::Length(d);
            const double t = len > 0 ? std::min(amount, len * 0.5) / len : 0;
            return along[q] = AddVertex(s, inV, p.verts[v] + d * t);
        };
        for (uint32_t f = 0; f < ext.faces.size(); ++f) {
            if (!live[f]) continue;
            const Face& F = ext.faces[f];
            int k = -1;
            for (int i = 0; i < 3; ++i) if (F.v[i] == v) k = i;
            if (k < 0) continue;
            const uint32_t a = F.v[(k + 1) % 3], b = F.v[(k + 2) % 3];
            const uint32_t na = onEdge(a), nb = onEdge(b);
            auto ev = [&](int side) { return (F.edgeVisible >> side) & 1u; };
            // (v, a, b) becomes the cap piece (v, na, nb) and the quad na, a, b, nb.
            ReplaceFace(s, inF, f, {MakeFace(F, v, na, nb, 2u), MakeFace(F, na, a, b, ev(k) | (ev((k + 1) % 3) << 1)),
                                    MakeFace(F, na, b, nb, 2u | 4u)});
        }
        s.deletedFaces = SortedUnique(s.deletedFaces);
        // The vertex moves to the new vertices' centre: the cap.
        math::Vec3 c{0, 0, 0};
        for (const auto& [q, nv] : along) c = c + s.addedVertices[nv - inV];
        if (!along.empty()) s.moves = AddToMoves(s.moves, {v}, c * (1.0 / static_cast<double>(along.size())) - p.verts[v]);
    }
    return s;
}

std::vector<std::vector<uint32_t>> EditMeshModifier::ExplodeGroups(const Mesh& input, double angle) const {
    const Mesh p = Positions(input), ext = Extended(input);
    const std::vector<char> live = LiveFaces(ext, GetState());
    std::vector<uint32_t> faces;
    for (uint32_t f : selectedFaces_) if (f < ext.faces.size() && live[f]) faces.push_back(f);
    std::map<EdgeKey, std::vector<uint32_t>> byEdge;
    for (uint32_t f : faces) for (int k = 0; k < 3; ++k) byEdge[Key(ext.faces[f].v[k], ext.faces[f].v[(k + 1) % 3])].push_back(f);
    const double cosLimit = std::cos(angle * 3.14159265358979323846 / 180.0);
    std::map<uint32_t, int> group;
    std::vector<std::vector<uint32_t>> out;
    for (uint32_t start : faces) {
        if (group.count(start)) continue;
        out.push_back({});
        std::vector<uint32_t> todo{start};
        group[start] = static_cast<int>(out.size()) - 1;
        while (!todo.empty()) {
            const uint32_t f = todo.back();
            todo.pop_back();
            out.back().push_back(f);
            const math::Vec3 nf = math::Normalize(FaceNormal(p, ext.faces[f]));
            for (int k = 0; k < 3; ++k)
                for (uint32_t g : byEdge[Key(ext.faces[f].v[k], ext.faces[f].v[(k + 1) % 3])]) {
                    if (group.count(g)) continue;
                    if (math::Dot(nf, math::Normalize(FaceNormal(p, ext.faces[g]))) + 1e-12 < cosLimit) continue;
                    group[g] = static_cast<int>(out.size()) - 1;
                    todo.push_back(g);
                }
        }
        std::sort(out.back().begin(), out.back().end());
    }
    return out;
}

EditMeshModifier::State EditMeshModifier::AfterExplodingToElements(const Mesh& input, double angle) const {
    State s = GetState();
    const Mesh p = Positions(input), ext = Extended(input);
    const size_t inV = input.verts.size(), inF = input.faces.size();
    const auto groups = ExplodeGroups(input, angle);
    // Every group after the first gets its own copies of the vertices it shares with others.
    std::map<uint32_t, int> firstGroup;
    std::vector<uint32_t> made;
    for (size_t gi = 0; gi < groups.size(); ++gi) {
        std::map<uint32_t, uint32_t> copy;
        for (uint32_t f : groups[gi]) {
            Face F = ext.faces[f];
            bool changed = false;
            for (uint32_t& v : F.v) {
                auto it = firstGroup.find(v);
                if (it == firstGroup.end()) { firstGroup[v] = static_cast<int>(gi); continue; }
                if (it->second == static_cast<int>(gi)) continue;
                if (!copy.count(v)) copy[v] = AddVertex(s, inV, p.verts[v]);
                v = copy[v];
                changed = true;
            }
            if (changed) { const auto m = ReplaceFace(s, inF, f, {F}); made.insert(made.end(), m.begin(), m.end()); }
            else made.push_back(f);
        }
    }
    s.deletedFaces = SortedUnique(s.deletedFaces);
    s.selectedFaces = SortedUnique(made);
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterAttaching(const Mesh& input, const Mesh& other) const {
    State s = GetState();
    const uint32_t base = static_cast<uint32_t>(input.verts.size() + s.addedVertices.size());
    s.addedVertices.insert(s.addedVertices.end(), other.verts.begin(), other.verts.end());
    for (Face f : other.faces) {
        for (uint32_t& v : f.v) v += base;
        s.addedFaces.push_back(f);
    }
    return s;
}

// ---- Interactive tools (10.9) -----------------------------------------------------------------

EditMeshModifier::State EditMeshModifier::AfterCreatingVertex(const Mesh& input, const math::Vec3& point) const {
    State s = GetState();
    s.selectedVertices = {AddVertex(s, input.verts.size(), point)};
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterCreatingFace(const Mesh& input, uint32_t a, uint32_t b, uint32_t c,
                                                            const math::Vec3& towards) const {
    State s = GetState();
    const Mesh p = Positions(input);
    if (a >= p.verts.size() || b >= p.verts.size() || c >= p.verts.size() || a == b || b == c || a == c) return s;
    if (math::Dot(math::Cross(p.verts[b] - p.verts[a], p.verts[c] - p.verts[a]), towards) < 0) std::swap(b, c);
    Face f{};
    f.v[0] = a; f.v[1] = b; f.v[2] = c;
    f.edgeVisible = 7;
    f.smoothing = 1;
    s.addedFaces.push_back(f);
    s.selectedFaces = {static_cast<uint32_t>(input.faces.size() + s.addedFaces.size() - 1)};
    return s;
}

namespace {
// Splits the live faces by new points on their edges (edge key -> new vertex): a face with
// points on two edges becomes three triangles along the line between them (`lineVisible`),
// a face with one point two triangles to the opposite corner (`fanVisible`).
void SplitByEdgePoints(EditMeshModifier::State& s, const Mesh& ext, const std::vector<char>& live, size_t inF,
                       const std::map<EdgeKey, uint32_t>& points, bool lineVisible, bool fanVisible,
                       std::vector<uint32_t>* made) {
    for (uint32_t f = 0; f < ext.faces.size(); ++f) {
        if (!live[f]) continue;
        const Face& F = ext.faces[f];
        int on[3], count = 0;
        uint32_t m[3] = {0, 0, 0};
        for (int k = 0; k < 3; ++k) {
            auto it = points.find(Key(F.v[k], F.v[(k + 1) % 3]));
            on[k] = it != points.end();
            if (on[k]) { m[k] = it->second; ++count; }
        }
        if (count == 0 || count == 3) continue;
        auto ev = [&](int k) { return (F.edgeVisible >> k) & 1u; };
        std::vector<Face> parts;
        if (count == 1) {
            const int k = on[0] ? 0 : on[1] ? 1 : 2;                 // the edge v_k -> v_k+1
            const uint32_t a = F.v[k], b = F.v[(k + 1) % 3], c = F.v[(k + 2) % 3];
            const uint32_t fan = fanVisible ? 1u : 0u;
            parts = {MakeFace(F, a, m[k], c, ev(k) | (fan << 1) | (ev((k + 2) % 3) << 2)),
                     MakeFace(F, m[k], b, c, ev(k) | (ev((k + 1) % 3) << 1) | (fan << 2))};
        } else {
            // The apex: the corner between the two edges with points.
            const int apex = !on[1] ? 0 : !on[2] ? 1 : 2;               // edges apex-1 and apex
            const int ka = apex, kb = (apex + 2) % 3;                    // apex -> next, prev -> apex
            const uint32_t v0 = F.v[apex], v1 = F.v[(apex + 1) % 3], v2 = F.v[(apex + 2) % 3];
            const uint32_t ma = m[ka], mb = m[kb];
            const uint32_t line = lineVisible ? 1u : 0u;
            parts = {MakeFace(F, v0, ma, mb, ev(ka) | (line << 1) | (ev(kb) << 2)),
                     MakeFace(F, ma, v1, v2, ev(ka) | (ev((apex + 1) % 3) << 1)),
                     MakeFace(F, ma, v2, mb, 0u | (ev(kb) << 1) | (line << 2))};
        }
        const auto ids = ReplaceFace(s, inF, f, parts);
        if (made) made->insert(made->end(), ids.begin(), ids.end());
    }
    s.deletedFaces = SortedUnique(s.deletedFaces);
}
} // namespace

EditMeshModifier::State EditMeshModifier::AfterSlicing(const Mesh& input, const math::Vec3& q, const math::Vec3& normal,
                                                       bool split, bool onlySelected) const {
    State s = GetState();
    const Mesh p = Positions(input), ext = Extended(input);
    const std::vector<char> live = LiveFaces(ext, s);
    const size_t inV = input.verts.size(), inF = input.faces.size();
    const math::Vec3 n = math::Normalize(normal);
    auto d = [&](uint32_t v) { return math::Dot(p.verts[v] - q, n); };
    // New vertices where the plane crosses an edge of a sliced face.
    std::map<EdgeKey, uint32_t> points;
    for (uint32_t f = 0; f < ext.faces.size(); ++f) {
        if (!live[f] || (onlySelected && !std::binary_search(selectedFaces_.begin(), selectedFaces_.end(), f))) continue;
        for (int k = 0; k < 3; ++k) {
            const uint32_t a = ext.faces[f].v[k], b = ext.faces[f].v[(k + 1) % 3];
            const double da = d(a), db = d(b);
            if (!(da * db < 0) || points.count(Key(a, b))) continue;
            points[Key(a, b)] = AddVertex(s, inV, p.verts[a] + (p.verts[b] - p.verts[a]) * (da / (da - db)));
        }
    }
    std::vector<uint32_t> made;
    SplitByEdgePoints(s, ext, live, inF, points, true, true, &made);
    if (split && !points.empty()) {
        // The positive side's faces get their own copies of the new vertices.
        std::map<uint32_t, uint32_t> copy;
        std::vector<uint32_t> newVerts;
        for (const auto& [key, v] : points) newVerts.push_back(v);
        std::sort(newVerts.begin(), newVerts.end());
        EditMeshModifier now = *this;
        now.SetState(s);
        const Mesh p2 = now.Positions(input);
        for (uint32_t id : made) {
            Face& F = s.addedFaces[id - inF];
            const math::Vec3 centre = (p2.verts[F.v[0]] + p2.verts[F.v[1]] + p2.verts[F.v[2]]) * (1.0 / 3.0);
            if (math::Dot(centre - q, n) <= 0) continue;
            for (uint32_t& v : F.v)
                if (std::binary_search(newVerts.begin(), newVerts.end(), v)) {
                    if (!copy.count(v)) copy[v] = AddVertex(s, inV, p2.verts[v]);
                    v = copy[v];
                }
        }
    }
    return s;
}

EditMeshModifier::State EditMeshModifier::AfterCutting(const Mesh& input, uint32_t edgeA, double tA, uint32_t edgeB, double tB) const {
    State s = GetState();
    const Mesh p = Positions(input), ext = Extended(input);
    const std::vector<char> live = LiveFaces(ext, s);
    if (edgeA / 3 != edgeB / 3 || edgeA == edgeB || edgeA / 3 >= ext.faces.size() || !live[edgeA / 3]) return s;
    const size_t inV = input.verts.size(), inF = input.faces.size();
    std::map<EdgeKey, uint32_t> points;
    for (const auto& [e, t] : {std::make_pair(edgeA, tA), std::make_pair(edgeB, tB)}) {
        uint32_t a, b;
        EdgeVertices(ext, e, a, b);
        const double u = std::min(0.999, std::max(0.001, t));
        points[Key(a, b)] = AddVertex(s, inV, p.verts[a] + (p.verts[b] - p.verts[a]) * u);
    }
    SplitByEdgePoints(s, ext, live, inF, points, true, false, nullptr);
    return s;
}

// ---- the gizmo (Phase 10.3) ---------------------------------------------------------------

namespace {
bool IsIdentity(const DeformModifier::Gizmo& g) {
    const PRS& t = g.transform;
    return t.position.x == 0 && t.position.y == 0 && t.position.z == 0 && t.rotation.x == 0 && t.rotation.y == 0 &&
           t.rotation.z == 0 && t.scale.x == 1 && t.scale.y == 1 && t.scale.z == 1 && g.center.x == 0 && g.center.y == 0 &&
           g.center.z == 0;
}
} // namespace

bool DeformModifier::Apply(Mesh& mesh) const {
    if (IsIdentity(gizmo_)) { Deform(mesh); return true; }        // exactly Phase 7's path
    const math::Mat4 g = gizmo_.transform.Matrix(), inv = math::AffineInverse(g);
    for (auto& v : mesh.verts) v = math::TransformPoint(inv, v) - gizmo_.center;
    Deform(mesh);
    for (auto& v : mesh.verts) v = math::TransformPoint(g, v + gizmo_.center);
    return true;
}

math::Vec3 DeformModifier::CenterPoint() const { return math::TransformPoint(gizmo_.transform.Matrix(), gizmo_.center); }

std::vector<std::pair<math::Vec3, math::Vec3>> DeformModifier::GizmoLines(const Mesh& input, int segments) const {
    std::vector<std::pair<math::Vec3, math::Vec3>> out;
    if (input.verts.empty() || segments < 1) return out;
    const math::Mat4 g = gizmo_.transform.Matrix(), inv = math::AffineInverse(g);
    // The input's bounding box in gizmo space.
    math::Vec3 lo{1e300, 1e300, 1e300}, hi{-1e300, -1e300, -1e300};
    for (const auto& v : input.verts) {
        const math::Vec3 p = math::TransformPoint(inv, v) - gizmo_.center;
        lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
        hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
    }
    // Its twelve edges in pieces, as object-space points, deformed together (so the
    // modifier sees the box's own extent, which equals the input's).
    Mesh lattice;
    for (int c = 0; c < 8; ++c)
        for (int k = 1; k <= 4; k <<= 1) {
            if (c & k) continue;
            for (int i = 0; i <= segments; ++i) {
                const double t = static_cast<double>(i) / segments;
                const int lo3[3] = {c & 1, c & 2, c & 4};
                auto at = [&](int bit, double lov, double hiv, int set) {
                    const double base = (set ? hiv : lov);
                    return (k == bit) ? lov + (hiv - lov) * t : base;
                };
                const math::Vec3 p{at(1, lo.x, hi.x, lo3[0]), at(2, lo.y, hi.y, lo3[1]), at(4, lo.z, hi.z, lo3[2])};
                lattice.verts.push_back(math::TransformPoint(g, p + gizmo_.center));
            }
        }
    Apply(lattice);
    const size_t perEdge = static_cast<size_t>(segments) + 1;
    for (size_t e = 0; e + perEdge <= lattice.verts.size(); e += perEdge)
        for (size_t i = 0; i < static_cast<size_t>(segments); ++i) out.push_back({lattice.verts[e + i], lattice.verts[e + i + 1]});
    return out;
}

} // namespace scene
