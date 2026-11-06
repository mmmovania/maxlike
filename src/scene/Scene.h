#pragma once
#include "scene/Node.h"
#include <memory>
#include <string>
#include <vector>

// Owns every node and keeps the hierarchy consistent.
namespace scene {

class Scene {
public:
    // Creates a node (top level) with a new id. The scene owns it; the pointer stays
    // valid until the node is deleted.
    Node* CreateNode(const std::string& name);

    // Recreates a node with a given id (for undo). Returns nullptr if the id is in use.
    Node* RestoreNode(Node::Id id, const std::string& name);

    // R3's naming: base name plus the next free two-digit number (Box01, Box02, ...).
    std::string UniqueName(const std::string& base) const;

    // Link `child` under `parent` (nullptr = top level). Refuses cycles. The child's
    // world placement is kept, as Max's Select and Link does.
    bool Link(Node* child, Node* parent);

    // Delete a node; its children move to the node's parent, keeping their world placement.
    void DeleteNode(Node* node);                 // also drops it from the selection

    // The selection, by node id, in the order nodes were selected. Changes that should
    // be undoable go through SelectCommand (scene/Undo.h).
    const std::vector<Node::Id>& Selection() const { return selection_; }
    void SetSelection(std::vector<Node::Id> ids); // unknown ids and duplicates are dropped
    bool IsSelected(Node::Id id) const;

    Node* Find(const std::string& name) const;
    Node* FindById(Node::Id id) const;
    const std::vector<std::unique_ptr<Node>>& Nodes() const { return nodes_; }
    std::vector<Node*> TopLevel() const;

    // Sets a node's world placement (keeping its parent) by solving for its local PRS.
    static void SetWorldMatrix(Node* node, const math::Mat4& world);

private:
    std::vector<std::unique_ptr<Node>> nodes_;
    Node::Id nextId_ = 1;
    std::vector<Node::Id> selection_;
};

} // namespace scene
