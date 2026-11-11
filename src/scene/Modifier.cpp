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
