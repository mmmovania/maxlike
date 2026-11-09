#pragma once
#include "scene/Scene.h"
#include "scene/Modifier.h"
#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <vector>

// Undo and redo with the command pattern: every change to the scene is a Command that
// knows how to do and undo itself. Commands refer to nodes by id (never by pointer),
// so they stay valid when an undo deletes and a redo recreates a node.
namespace scene {

class Command {
public:
    virtual ~Command() = default;
    virtual std::string Name() const = 0;   // shown as "Undo <name>" in the Edit menu
    virtual void Do(Scene& scene) = 0;
    virtual void Undo(Scene& scene) = 0;
};

class UndoStack {
public:
    // R3's default number of undo levels (Preferences > General: Scene Undo Levels).
    // NOTE: the default of 20 is from memory, not verified.
    explicit UndoStack(size_t levels = 20) : levels_(levels) {}

    // Do the command and record it. Clears the redo list.
    void Execute(std::unique_ptr<Command> command, Scene& scene);
    bool Undo(Scene& scene);
    bool Redo(Scene& scene);

    bool CanUndo() const { return !undo_.empty(); }
    bool CanRedo() const { return !redo_.empty(); }
    std::string UndoName() const { return CanUndo() ? undo_.back()->Name() : std::string(); }
    std::string RedoName() const { return CanRedo() ? redo_.back()->Name() : std::string(); }
    size_t UndoCount() const { return undo_.size(); }

    // "Saved" bookkeeping: IsClean() is true when the scene matches the last MarkClean().
    void MarkClean() { cleanAt_ = position_; cleanValid_ = true; }
    bool IsClean() const { return cleanValid_ && cleanAt_ == position_; }
    void Clear() { undo_.clear(); redo_.clear(); position_ = 0; cleanValid_ = false; }

private:
    size_t levels_;
    std::deque<std::unique_ptr<Command>> undo_;
    std::vector<std::unique_ptr<Command>> redo_;
    long long position_ = 0;      // +1 per Execute/Redo, -1 per Undo
    long long cleanAt_ = 0;
    bool cleanValid_ = true;      // a new, empty scene counts as clean
};

// ---- commands -------------------------------------------------------------------

// Create a node showing `object` (Max's "Create").
class CreateNodeCommand : public Command {
public:
    CreateNodeCommand(std::string name, std::shared_ptr<Object> object, uint32_t wireColor, PRS transform)
        : name_(std::move(name)), object_(std::move(object)), color_(wireColor), transform_(transform) {}
    std::string Name() const override { return "Create"; }
    void Do(Scene& scene) override;
    void Undo(Scene& scene) override;
    Node::Id CreatedId() const { return id_; }

private:
    std::string name_;
    std::shared_ptr<Object> object_;
    uint32_t color_;
    PRS transform_;
    Node::Id id_ = 0;   // assigned on the first Do, reused on redo
};

// Delete a node. Its children move to its parent (keeping their world placement);
// undo puts everything back exactly, including the children's own transforms.
class DeleteNodeCommand : public Command {
public:
    explicit DeleteNodeCommand(Node::Id id) : id_(id) {}
    std::string Name() const override { return "Delete"; }
    void Do(Scene& scene) override;
    void Undo(Scene& scene) override;

private:
    struct ChildState { Node::Id id; PRS transform; };
    Node::Id id_;
    // Snapshot taken in Do.
    std::string name_;
    std::shared_ptr<Object> object_;
    uint32_t color_ = 0;
    PRS transform_, offset_;
    Node::Id parent_ = 0;
    std::vector<ChildState> children_;
};

// Set a node's transform (Move, Rotate, Scale).
class TransformCommand : public Command {
public:
    TransformCommand(Node::Id id, PRS before, PRS after, std::string name = "Move")
        : id_(id), before_(before), after_(after), name_(std::move(name)) {}
    std::string Name() const override { return name_; }
    void Do(Scene& scene) override;
    void Undo(Scene& scene) override;

private:
    Node::Id id_;
    PRS before_, after_;
    std::string name_;
};

// Change one parameter of an object (e.g. a Box's height) or of a modifier (a Bend's
// angle); affects every instance.
class SetParamCommand : public Command {
public:
    SetParamCommand(std::shared_ptr<Object> object, size_t index, double after)
        : block_(&object->Params()), owner_(std::move(object)), index_(index), after_(after) {}
    SetParamCommand(std::shared_ptr<Modifier> modifier, size_t index, double after)
        : block_(&modifier->Params()), owner_(std::move(modifier)), index_(index), after_(after) {}
    std::string Name() const override { return "Parameter Change"; }
    void Do(Scene& scene) override;
    void Undo(Scene& scene) override;

private:
    ParamBlock* block_;                 // owned by owner_, which this command keeps alive
    std::shared_ptr<void> owner_;
    size_t index_;
    double before_ = 0, after_;
};

// Add a modifier on top of a node's stack. A plain object first becomes the base of a new
// Derived Object (undo restores the plain object).
class AddModifierCommand : public Command {
public:
    AddModifierCommand(Node::Id node, std::shared_ptr<Modifier> modifier) : node_(node), modifier_(std::move(modifier)) {}
    std::string Name() const override { return "Add Modifier"; }
    void Do(Scene& scene) override;
    void Undo(Scene& scene) override;

private:
    Node::Id node_;
    std::shared_ptr<Modifier> modifier_;
    std::shared_ptr<DerivedObject> derived_;
    std::shared_ptr<Object> original_;   // set when Do wrapped a plain object
};

// Remove the modifier at `index` (0 = bottom of the stack) from a node's stack.
class RemoveModifierCommand : public Command {
public:
    RemoveModifierCommand(Node::Id node, size_t index) : node_(node), index_(index) {}
    std::string Name() const override { return "Delete Modifier"; }
    void Do(Scene& scene) override;
    void Undo(Scene& scene) override;

private:
    Node::Id node_;
    size_t index_;
    std::shared_ptr<DerivedObject> derived_;
    std::shared_ptr<Modifier> removed_;
};

// Edit Mesh at Vertex level: change the vertex selection.
class SelectVerticesCommand : public Command {
public:
    SelectVerticesCommand(std::shared_ptr<EditMeshModifier> mod, std::vector<uint32_t> after)
        : mod_(std::move(mod)), after_(std::move(after)) {}
    std::string Name() const override { return "Select"; }
    void Do(Scene&) override { before_ = mod_->SelectedVertices(); mod_->SetSelectedVertices(after_); }
    void Undo(Scene&) override { mod_->SetSelectedVertices(before_); }

private:
    std::shared_ptr<EditMeshModifier> mod_;
    std::vector<uint32_t> before_, after_;
};

// Any Edit Mesh change as one undo step (Phase 9.4: face selection, Delete): the whole
// editing state before and after.
class EditMeshStateCommand : public Command {
public:
    EditMeshStateCommand(std::string name, std::shared_ptr<EditMeshModifier> mod, EditMeshModifier::State after)
        : name_(std::move(name)), mod_(std::move(mod)), after_(std::move(after)) {}
    std::string Name() const override { return name_; }
    void Do(Scene&) override { before_ = mod_->GetState(); mod_->SetState(after_); }
    void Undo(Scene&) override { mod_->SetState(before_); }

private:
    std::string name_;
    std::shared_ptr<EditMeshModifier> mod_;
    EditMeshModifier::State before_, after_;
};

// Edit Mesh at Vertex level: replace the vertex moves (one drag of the selection).
class MoveVerticesCommand : public Command {
public:
    MoveVerticesCommand(std::shared_ptr<EditMeshModifier> mod, std::vector<EditMeshModifier::Move> after)
        : mod_(std::move(mod)), after_(std::move(after)) {}
    std::string Name() const override { return "Move"; }
    void Do(Scene&) override { before_ = mod_->Moves(); mod_->SetMoves(after_); }
    void Undo(Scene&) override { mod_->SetMoves(before_); }

private:
    std::shared_ptr<EditMeshModifier> mod_;
    std::vector<EditMeshModifier::Move> before_, after_;
};

// Turn a modifier on or off (the stack's light bulb).
class EnableModifierCommand : public Command {
public:
    EnableModifierCommand(std::shared_ptr<Modifier> modifier, bool on) : modifier_(std::move(modifier)), on_(on) {}
    std::string Name() const override { return "Modifier On/Off"; }
    void Do(Scene&) override { before_ = modifier_->Enabled(); modifier_->SetEnabled(on_); }
    void Undo(Scene&) override { modifier_->SetEnabled(before_); }

private:
    std::shared_ptr<Modifier> modifier_;
    bool on_, before_ = true;
};

// Link a node under a parent (0 = unlink), keeping its world placement.
class LinkCommand : public Command {
public:
    LinkCommand(Node::Id child, Node::Id parent) : child_(child), parent_(parent) {}
    std::string Name() const override { return parent_ ? "Link" : "Unlink"; }
    void Do(Scene& scene) override;
    void Undo(Scene& scene) override;

private:
    Node::Id child_, parent_;
    Node::Id oldParent_ = 0;
    PRS oldTransform_;
};

// Rename a node / change its wireframe colour.
class RenameCommand : public Command {
public:
    RenameCommand(Node::Id id, std::string name) : id_(id), after_(std::move(name)) {}
    std::string Name() const override { return "Rename"; }
    void Do(Scene& scene) override;
    void Undo(Scene& scene) override;

private:
    Node::Id id_;
    std::string before_, after_;
};

class WireColorCommand : public Command {
public:
    WireColorCommand(Node::Id id, uint32_t color) : id_(id), after_(color) {}
    std::string Name() const override { return "Color Change"; }
    void Do(Scene& scene) override;
    void Undo(Scene& scene) override;

private:
    Node::Id id_;
    uint32_t before_ = 0, after_;
};

// Change the selection (R3 lists it in Edit > Undo as "Select").
class SelectCommand : public Command {
public:
    explicit SelectCommand(std::vector<Node::Id> after) : after_(std::move(after)) {}
    std::string Name() const override { return "Select"; }
    void Do(Scene& scene) override { before_ = scene.Selection(); scene.SetSelection(after_); }
    void Undo(Scene& scene) override { scene.SetSelection(before_); }

private:
    std::vector<Node::Id> before_, after_;
};

// Delete every selected node as one step (Edit > Delete, the Delete key); undo brings
// them back, selected.
class DeleteSelectionCommand : public Command {
public:
    std::string Name() const override { return "Delete"; }
    void Do(Scene& scene) override;
    void Undo(Scene& scene) override;

private:
    std::vector<Node::Id> selected_;
    std::vector<std::unique_ptr<DeleteNodeCommand>> parts_;
};

// Nodes added to the scene outside the undo stack (File > Merge): already present when
// executed. Undo deletes them, children first; redo restores them, parents first. `ids`
// must list parents before their children.
class AddedNodesCommand : public Command {
public:
    AddedNodesCommand(std::string name, std::vector<Node::Id> ids) : name_(std::move(name)), ids_(std::move(ids)) {}
    std::string Name() const override { return name_; }
    void Do(Scene& scene) override;
    void Undo(Scene& scene) override;

private:
    std::string name_;
    std::vector<Node::Id> ids_;
    bool first_ = true;
    std::vector<std::unique_ptr<DeleteNodeCommand>> deleted_;   // in deletion order
};

// Several commands as one undo step (e.g. creating an object, then adjusting it while
// dragging). Undone in reverse order.
class MacroCommand : public Command {
public:
    explicit MacroCommand(std::string name) : name_(std::move(name)) {}
    void Add(std::unique_ptr<Command> c) { parts_.push_back(std::move(c)); }
    std::string Name() const override { return name_; }
    void Do(Scene& scene) override;
    void Undo(Scene& scene) override;

private:
    std::string name_;
    std::vector<std::unique_ptr<Command>> parts_;
};

} // namespace scene
