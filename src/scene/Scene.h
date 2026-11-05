#pragma once
#include "scene/Node.h"
#include <memory>
#include <string>
#include <vector>

// Owns every node and keeps the hierarchy consistent.
namespace scene {

class Scene {
public:
    // Creates a node (top level). The scene owns it; the pointer stays valid until
    // the node is deleted.
    Node* CreateNode(const std::string& name);

    // R3's naming: base name plus the next free two-digit number (Box01, Box02, ...).
    std::string UniqueName(const std::string& base) const;

    // Link `child` under `parent` (nullptr = top level). Refuses cycles. The child's
    // world placement is kept, as Max's Select and Link does.
    bool Link(Node* child, Node* parent);

    // Delete a node; its children move to the node's parent, keeping their world placement.
    void DeleteNode(Node* node);

    Node* Find(const std::string& name) const;
    const std::vector<std::unique_ptr<Node>>& Nodes() const { return nodes_; }
    std::vector<Node*> TopLevel() const;

private:
    static void SetWorldMatrix(Node* node, const math::Mat4& world);
    std::vector<std::unique_ptr<Node>> nodes_;
};

} // namespace scene
