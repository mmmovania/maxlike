#pragma once
#include "scene/Node.h"
#include "scene/Object.h"
#include "scene/ParamBlock.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// The modifier stack (max-format.md §8): a Derived Object holds a base object and
// modifiers applied from the bottom up. Several nodes may share one Derived Object
// (instances.max: seven nodes, one Box + Edit Mesh).
namespace scene {

class Modifier {
public:
    virtual ~Modifier() = default;
    virtual std::string ClassName() const = 0;
    // Change the mesh in place. Returns false if the modifier could not apply (the
    // mesh is then left unchanged).
    virtual bool Apply(Mesh& mesh) const = 0;

    bool Enabled() const { return enabled_; }
    void SetEnabled(bool on) { enabled_ = on; }

    // Parameters, addressed by index like a primitive's (empty for modifiers without).
    ParamBlock& Params() { return params_; }
    const ParamBlock& Params() const { return params_; }

protected:
    ParamBlock params_;

private:
    bool enabled_ = true;
};

// Bend, Taper and Twist (Phase 7). They deform the mesh in object space about the
// object's pivot, along a chosen axis (0 X, 1 Y, 2 Z); h is a vertex's coordinate along
// that axis and L the mesh's extent along it. With limits on, the effect stops at the
// lower and upper limits (h values) and the rest follows rigidly.
// NOTE: the parameter indices follow memory of R3's parameter blocks, and the Twist
// bias and Taper curve formulas are approximations; all unverified.
// Bend, Taper and Twist share a gizmo and a centre (Phase 10.3): points are taken into
// the gizmo's frame, relative to the centre (gizmo space), deformed there, and brought
// back. With the identity gizmo and a zero centre nothing is transformed, so the results
// are exactly Phase 7's (verified against R3.1).
class DeformModifier : public Modifier {
public:
    bool Apply(Mesh& mesh) const final;
    struct Gizmo {
        PRS transform;                       // the gizmo in the object's space
        math::Vec3 center{0, 0, 0};          // the centre, in the gizmo's space
    };
    const Gizmo& GetGizmo() const { return gizmo_; }
    void SetGizmo(const Gizmo& g) { gizmo_ = g; }
    // The gizmo as R3 draws it: the input's bounding box in gizmo space, each edge in
    // `segments` pieces, deformed by this modifier; pairs of object-space points.
    std::vector<std::pair<math::Vec3, math::Vec3>> GizmoLines(const Mesh& input, int segments = 8) const;
    math::Vec3 CenterPoint() const;          // the centre in object space

protected:
    virtual void Deform(Mesh& mesh) const = 0;   // in gizmo space

private:
    Gizmo gizmo_;
};

class BendModifier : public DeformModifier {
public:
    enum { kAngle = 0, kDirection, kAxis, kLimit, kLower, kUpper };
    BendModifier();
    std::string ClassName() const override { return "Bend"; }
protected:
    void Deform(Mesh& mesh) const override;
};

class TaperModifier : public DeformModifier {
public:
    enum { kAmount = 0, kCurve, kAxis, kEffect, kSymmetry, kLimit, kLower, kUpper };
    // kEffect: which perpendicular axes are scaled: 0 the first, 1 the second, 2 both (as
    // the rollout's X / Y / XY radios; R3's default is both).
    TaperModifier();
    std::string ClassName() const override { return "Taper"; }
protected:
    void Deform(Mesh& mesh) const override;
};

class TwistModifier : public DeformModifier {
public:
    enum { kAngle = 0, kBias, kAxis, kLimit, kLower, kUpper };
    TwistModifier();
    std::string ClassName() const override { return "Twist"; }
protected:
    void Deform(Mesh& mesh) const override;
};

// Edit Mesh, vertex moves only (local data 0x4000: 0x0100 vertex count, 0x0140 moves).
// Max records how many vertices the mesh had; if the mesh below now has a different
// count, the indices no longer mean the same vertices and the moves are not applied.
// NOTE: Edit Mesh topology changes (delete, extrude) and smoothing edits are not decoded.
class EditMeshModifier : public Modifier {
public:
    struct Move { uint32_t vertex; math::Vec3 delta; };

    // Settings shown in Edit Mesh's rollouts (maxlike's own indices: R3 keeps these
    // outside a parameter block).
    enum { kIgnoreBackfacing = 0, kPlanarThresh, kExtrudeAmount, kChamferAmount, kSplit, kRefineEnds,
           kWeldThreshold, kNormalLocal, kWeldTargetPixels, kTessTension, kTessByFace, kExplodeAngle,
           kExplodeToElements,
           // Soft Selection (Phase 10.6), after the others so no index moves.
           kUseSoftSelection, kEdgeDistance, kEdgeIterations, kAffectBackfacing, kFalloff, kPinch, kBubble };
    EditMeshModifier(size_t expectedVertexCount, std::vector<Move> moves);
    std::string ClassName() const override { return "Edit Mesh"; }
    bool Apply(Mesh& mesh) const override;
    const std::vector<Move>& Moves() const { return moves_; }
    void SetMoves(std::vector<Move> moves) { moves_ = std::move(moves); }
    size_t ExpectedVertexCount() const { return expected_; }

    // Sub-object selection at Vertex level (Phase 7): vertex indices, sorted, unique.
    const std::vector<uint32_t>& SelectedVertices() const { return selected_; }
    void SetSelectedVertices(std::vector<uint32_t> v);

    // The moves after adding `delta` (object space) to every vertex in `vertices`.
    static std::vector<Move> AddToMoves(const std::vector<Move>& moves, const std::vector<uint32_t>& vertices,
                                        const math::Vec3& delta);
    // The same with one offset per vertex (sub-object Rotate and Scale, Phase 10.5).
    static std::vector<Move> AddToMoves(const std::vector<Move>& moves, const std::vector<uint32_t>& vertices,
                                        const std::vector<math::Vec3>& deltas);

    // Phase 9.4: Face level and Delete. Every index refers to the mesh coming into Edit
    // Mesh, so edits never renumber each other; Edited() gives the output with maps back.
    const std::vector<uint32_t>& SelectedFaces() const { return selectedFaces_; }
    void SetSelectedFaces(std::vector<uint32_t> f);
    // Edge level (9.5): edges as input face * 3 + side (scene::EdgeId).
    const std::vector<uint32_t>& SelectedEdges() const { return selectedEdges_; }
    void SetSelectedEdges(std::vector<uint32_t> e);
    const std::vector<uint32_t>& DeletedFaces() const { return deletedFaces_; }
    const std::vector<uint32_t>& DeletedVertices() const { return deletedVerts_; }
    void SetDeleted(std::vector<uint32_t> faces, std::vector<uint32_t> vertices);

    // Everything Edit Mesh has edited, as one value (for undo, copies and files).
    struct State {
        std::vector<Move> moves;
        std::vector<uint32_t> selectedVertices, selectedFaces, deletedFaces, deletedVertices;
        std::vector<uint32_t> selectedEdges;
        std::vector<uint32_t> hiddenFaces, hiddenVertices;          // 9.6
        std::vector<std::pair<uint32_t, uint32_t>> welds;           // 9.6: vertex -> the vertex it merged into
    };
    State GetState() const {
        return {moves_, selected_, selectedFaces_, deletedFaces_, deletedVerts_, selectedEdges_, hiddenFaces_, hiddenVerts_, welds_};
    }
    void SetState(const State& s);

    // The input with the moves applied, then the deleted faces and vertices removed (a
    // face goes with any of its vertices). vertexMap / faceMap: output index -> input index.
    Mesh Edited(const Mesh& input, std::vector<uint32_t>* vertexMap = nullptr, std::vector<uint32_t>* faceMap = nullptr) const;

    // The state after Delete: the selected faces (and, if `isolated`, vertices no remaining
    // face uses), or the selected vertices with their faces. Selections lose what is deleted.
    State AfterDeletingFaces(const Mesh& input, bool isolated) const;
    State AfterDeletingVertices() const;
    // Deleting edges deletes the faces on both sides of each (and, if `isolated`, the
    // vertices no remaining face uses). NOTE: R3's behaviour assumed.
    State AfterDeletingEdges(const Mesh& input, bool isolated) const;

    // Phase 9.6. Hide: at Vertex level (`faces` false) the selected vertices' markers, at
    // Face, Polygon and Element level the selected faces (left out of the output; R3 keeps
    // them flagged in the mesh, NOTE: no visible difference without a renderer). Unhide
    // All: every hidden vertex or face. Selections lose what is hidden.
    State AfterHiding(bool faces) const;
    State AfterUnhidingAll(bool faces) const;
    const std::vector<uint32_t>& HiddenFaces() const { return hiddenFaces_; }
    const std::vector<uint32_t>& HiddenVertices() const { return hiddenVerts_; }
    // Weld Selected: selected vertices closer than `threshold` (in the input's space, after
    // the moves) merge into one at their average; faces that collapse disappear. `merged`:
    // how many vertices were merged away (0: nothing within the threshold).
    State AfterWelding(const Mesh& input, double threshold, size_t* merged) const;
    const std::vector<std::pair<uint32_t, uint32_t>>& Welds() const { return welds_; }

    // Phase 9.7. Weld Target: vertex `from` welded into `to` (which stays where it is).
    State AfterWeldingTarget(uint32_t from, uint32_t to) const;
    // Detach: the faces `inputFaces` of the output, as a separate compact mesh (vertices
    // renumbered, smoothing, material IDs and edges kept).
    Mesh Extract(const Mesh& input, const std::vector<uint32_t>& inputFaces) const;

private:
    size_t expected_;
    std::vector<Move> moves_;
    std::vector<uint32_t> selected_;
    std::vector<uint32_t> selectedFaces_, deletedFaces_, deletedVerts_, selectedEdges_;
    std::vector<uint32_t> hiddenFaces_, hiddenVerts_;
    std::vector<std::pair<uint32_t, uint32_t>> welds_;
};

// A base object with a stack of modifiers. Index 0 is the bottom of the stack (applied
// first, right above the base object), as Max lists them from the top in its UI.
class DerivedObject : public Object {
public:
    explicit DerivedObject(std::shared_ptr<Object> base) : base_(std::move(base)) {}

    std::string ClassName() const override { return base_ ? base_->ClassName() : "Derived Object"; }
    Mesh Evaluate() const override;
    // The mesh after the base and the first `count` modifiers (Edit Mesh edits the mesh as
    // it is at its own level, whatever is above it).
    Mesh EvaluateUpTo(size_t count) const;

    const std::shared_ptr<Object>& Base() const { return base_; }
    void AddModifier(std::shared_ptr<Modifier> modifier) { modifiers_.push_back(std::move(modifier)); }
    void InsertModifier(size_t index, std::shared_ptr<Modifier> modifier) {
        modifiers_.insert(modifiers_.begin() + static_cast<std::ptrdiff_t>(std::min(index, modifiers_.size())), std::move(modifier));
    }
    void RemoveModifierAt(size_t index) {
        if (index < modifiers_.size()) modifiers_.erase(modifiers_.begin() + static_cast<std::ptrdiff_t>(index));
    }
    const std::vector<std::shared_ptr<Modifier>>& Modifiers() const { return modifiers_; }
    // Names of modifiers that could not apply in the last Evaluate (for a status note).
    const std::vector<std::string>& Skipped() const { return skipped_; }

private:
    std::shared_ptr<Object> base_;
    std::vector<std::shared_ptr<Modifier>> modifiers_;
    mutable std::vector<std::string> skipped_;
};

} // namespace scene
