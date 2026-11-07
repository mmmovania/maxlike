#pragma once
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
class BendModifier : public Modifier {
public:
    enum { kAngle = 0, kDirection, kAxis, kLimit, kLower, kUpper };
    BendModifier();
    std::string ClassName() const override { return "Bend"; }
    bool Apply(Mesh& mesh) const override;
};

class TaperModifier : public Modifier {
public:
    enum { kAmount = 0, kCurve, kAxis, kEffect, kSymmetry, kLimit, kLower, kUpper };
    // kEffect: which perpendicular axes are scaled: 0 the first, 1 the second, 2 both (as
    // the rollout's X / Y / XY radios; R3's default is both).
    TaperModifier();
    std::string ClassName() const override { return "Taper"; }
    bool Apply(Mesh& mesh) const override;
};

class TwistModifier : public Modifier {
public:
    enum { kAngle = 0, kBias, kAxis, kLimit, kLower, kUpper };
    TwistModifier();
    std::string ClassName() const override { return "Twist"; }
    bool Apply(Mesh& mesh) const override;
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
           kExplodeToElements };
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

private:
    size_t expected_;
    std::vector<Move> moves_;
    std::vector<uint32_t> selected_;
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
