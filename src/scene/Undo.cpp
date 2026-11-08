#include "scene/Undo.h"

namespace scene {

// ---- stack ------------------------------------------------------------------------

void UndoStack::Execute(std::unique_ptr<Command> command, Scene& scene) {
    command->Do(scene);
    undo_.push_back(std::move(command));
    redo_.clear();
    // If the clean state was in the redo list, it can never be reached again.
    if (cleanValid_ && cleanAt_ > position_) cleanValid_ = false;
    ++position_;
    while (undo_.size() > levels_) undo_.pop_front();   // forget the oldest steps
    // The oldest state still reachable by undoing; a saved state older than that is gone.
    const long long oldest = position_ - static_cast<long long>(undo_.size());
    if (cleanValid_ && cleanAt_ < oldest) cleanValid_ = false;
}

bool UndoStack::Undo(Scene& scene) {
    if (undo_.empty()) return false;
    std::unique_ptr<Command> c = std::move(undo_.back());
    undo_.pop_back();
    c->Undo(scene);
    redo_.push_back(std::move(c));
    --position_;
    return true;
}

bool UndoStack::Redo(Scene& scene) {
    if (redo_.empty()) return false;
    std::unique_ptr<Command> c = std::move(redo_.back());
    redo_.pop_back();
    c->Do(scene);
    undo_.push_back(std::move(c));
    ++position_;
    return true;
}

// ---- commands -------------------------------------------------------------------

void CreateNodeCommand::Do(Scene& scene) {
    Node* n = id_ ? scene.RestoreNode(id_, name_) : scene.CreateNode(name_);
    if (!n) return;
    id_ = n->GetId();
    n->SetObject(object_);
    n->SetWireColor(color_);
    n->Transform() = transform_;
}

void CreateNodeCommand::Undo(Scene& scene) {
    if (Node* n = scene.FindById(id_)) scene.DeleteNode(n);
}

void DeleteNodeCommand::Do(Scene& scene) {
    Node* n = scene.FindById(id_);
    if (!n) return;
    name_ = n->Name();
    object_ = n->GetObject();
    color_ = n->WireColor();
    transform_ = n->Transform();
    offset_ = n->ObjectOffset();
    parent_ = n->Parent() ? n->Parent()->GetId() : 0;
    children_.clear();
    for (Node* c : n->Children()) children_.push_back({c->GetId(), c->Transform()});
    scene.DeleteNode(n);
}

void DeleteNodeCommand::Undo(Scene& scene) {
    Node* n = scene.RestoreNode(id_, name_);
    if (!n) return;
    n->SetObject(object_);
    n->SetWireColor(color_);
    n->ObjectOffset() = offset_;
    if (Node* p = scene.FindById(parent_)) scene.Link(n, p);
    n->Transform() = transform_;              // exactly as before (Link may have rounded)
    for (const ChildState& cs : children_)
        if (Node* c = scene.FindById(cs.id)) {
            scene.Link(c, n);
            c->Transform() = cs.transform;    // the child's own local transform, exactly
        }
}

void TransformCommand::Do(Scene& scene) {
    if (Node* n = scene.FindById(id_)) n->Transform() = after_;
}

void TransformCommand::Undo(Scene& scene) {
    if (Node* n = scene.FindById(id_)) n->Transform() = before_;
}

void SetParamCommand::Do(Scene&) {
    before_ = block_->Float(index_);
    block_->Set(index_, after_);
}

void SetParamCommand::Undo(Scene&) {
    block_->Set(index_, before_);
}

void AddModifierCommand::Do(Scene& scene) {
    Node* n = scene.FindById(node_);
    if (!n) return;
    derived_ = std::dynamic_pointer_cast<DerivedObject>(n->GetObject());
    original_.reset();
    if (!derived_) {
        original_ = n->GetObject();
        derived_ = std::make_shared<DerivedObject>(original_);
        n->SetObject(derived_);
    }
    derived_->AddModifier(modifier_);
}

void AddModifierCommand::Undo(Scene& scene) {
    if (!derived_) return;
    const auto& mods = derived_->Modifiers();
    for (size_t i = mods.size(); i-- > 0;)
        if (mods[i] == modifier_) { derived_->RemoveModifierAt(i); break; }
    if (original_)
        if (Node* n = scene.FindById(node_)) n->SetObject(original_);
}

void RemoveModifierCommand::Do(Scene& scene) {
    Node* n = scene.FindById(node_);
    derived_ = n ? std::dynamic_pointer_cast<DerivedObject>(n->GetObject()) : nullptr;
    if (!derived_ || index_ >= derived_->Modifiers().size()) { derived_.reset(); return; }
    removed_ = derived_->Modifiers()[index_];
    derived_->RemoveModifierAt(index_);
}

void RemoveModifierCommand::Undo(Scene&) {
    if (derived_ && removed_) derived_->InsertModifier(index_, removed_);
}

void LinkCommand::Do(Scene& scene) {
    Node* c = scene.FindById(child_);
    if (!c) return;
    oldParent_ = c->Parent() ? c->Parent()->GetId() : 0;
    oldTransform_ = c->Transform();
    scene.Link(c, parent_ ? scene.FindById(parent_) : nullptr);
}

void LinkCommand::Undo(Scene& scene) {
    Node* c = scene.FindById(child_);
    if (!c) return;
    scene.Link(c, oldParent_ ? scene.FindById(oldParent_) : nullptr);
    c->Transform() = oldTransform_;
}

void RenameCommand::Do(Scene& scene) {
    if (Node* n = scene.FindById(id_)) { before_ = n->Name(); n->SetName(after_); }
}

void RenameCommand::Undo(Scene& scene) {
    if (Node* n = scene.FindById(id_)) n->SetName(before_);
}

void WireColorCommand::Do(Scene& scene) {
    if (Node* n = scene.FindById(id_)) { before_ = n->WireColor(); n->SetWireColor(after_); }
}

void WireColorCommand::Undo(Scene& scene) {
    if (Node* n = scene.FindById(id_)) n->SetWireColor(before_);
}

void DeleteSelectionCommand::Do(Scene& scene) {
    selected_ = scene.Selection();
    parts_.clear();
    for (Node::Id id : selected_) {
        parts_.push_back(std::make_unique<DeleteNodeCommand>(id));
        parts_.back()->Do(scene);
    }
}

void DeleteSelectionCommand::Undo(Scene& scene) {
    for (auto it = parts_.rbegin(); it != parts_.rend(); ++it) (*it)->Undo(scene);
    scene.SetSelection(selected_);
}

void MacroCommand::Do(Scene& scene) {
    for (auto& c : parts_) c->Do(scene);
}

void MacroCommand::Undo(Scene& scene) {
    for (auto it = parts_.rbegin(); it != parts_.rend(); ++it) (*it)->Undo(scene);
}

void AddedNodesCommand::Do(Scene& scene) {
    if (first_) { first_ = false; return; }                 // the nodes are already there
    for (auto it = deleted_.rbegin(); it != deleted_.rend(); ++it) (*it)->Undo(scene);   // parents first
    deleted_.clear();
}

void AddedNodesCommand::Undo(Scene& scene) {
    deleted_.clear();
    for (auto it = ids_.rbegin(); it != ids_.rend(); ++it) {   // children first
        auto d = std::make_unique<DeleteNodeCommand>(*it);
        d->Do(scene);
        deleted_.push_back(std::move(d));
    }
}

} // namespace scene
