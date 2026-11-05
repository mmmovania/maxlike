#include "scene/Scene.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace scene {

Node* Scene::CreateNode(const std::string& name) {
    nodes_.push_back(std::make_unique<Node>(name));
    return nodes_.back().get();
}

std::string Scene::UniqueName(const std::string& base) const {
    for (int n = 1;; ++n) {
        char suffix[16];
        std::snprintf(suffix, sizeof(suffix), "%02d", n);
        const std::string name = base + suffix;
        if (!Find(name)) return name;
    }
}

Node* Scene::Find(const std::string& name) const {
    for (const auto& n : nodes_)
        if (n->Name() == name) return n.get();
    return nullptr;
}

std::vector<Node*> Scene::TopLevel() const {
    std::vector<Node*> out;
    for (const auto& n : nodes_)
        if (!n->parent_) out.push_back(n.get());
    return out;
}

// Decompose a world matrix (rotation + scale + translation, no shear) back into the
// node's local PRS under its current parent.
void Scene::SetWorldMatrix(Node* node, const math::Mat4& world) {
    const math::Mat4 local = node->parent_ ? math::AffineInverse(node->parent_->WorldMatrix()) * world : world;
    PRS& t = node->transform_;
    t.position = {local.m[3][0], local.m[3][1], local.m[3][2]};
    math::Vec3 cols[3];
    for (int c = 0; c < 3; ++c) cols[c] = {local.m[c][0], local.m[c][1], local.m[c][2]};
    t.scale = {math::Length(cols[0]), math::Length(cols[1]), math::Length(cols[2])};
    // A mirrored matrix (negative determinant) cannot be a rotation: put the flip
    // into the X scale instead.
    if (math::Dot(math::Cross(cols[0], cols[1]), cols[2]) < 0) {
        t.scale.x = -t.scale.x;
        cols[0] = cols[0] * -1.0;
    }
    // Rotation matrix from the normalised columns -> quaternion.
    const math::Vec3 X = math::Normalize(cols[0]), Y = math::Normalize(cols[1]), Z = math::Normalize(cols[2]);
    const double tr = X.x + Y.y + Z.z;
    math::Quat q;
    if (tr > 0) {
        const double s = std::sqrt(tr + 1.0) * 2;
        q = {(Y.z - Z.y) / s, (Z.x - X.z) / s, (X.y - Y.x) / s, 0.25 * s};
    } else if (X.x > Y.y && X.x > Z.z) {
        const double s = std::sqrt(1.0 + X.x - Y.y - Z.z) * 2;
        q = {0.25 * s, (Y.x + X.y) / s, (Z.x + X.z) / s, (Y.z - Z.y) / s};
    } else if (Y.y > Z.z) {
        const double s = std::sqrt(1.0 + Y.y - X.x - Z.z) * 2;
        q = {(Y.x + X.y) / s, 0.25 * s, (Z.y + Y.z) / s, (Z.x - X.z) / s};
    } else {
        const double s = std::sqrt(1.0 + Z.z - X.x - Y.y) * 2;
        q = {(Z.x + X.z) / s, (Z.y + Y.z) / s, 0.25 * s, (X.y - Y.x) / s};
    }
    t.rotation = q.Normalized();
}

bool Scene::Link(Node* child, Node* parent) {
    if (!child || child == parent) return false;
    for (Node* p = parent; p; p = p->parent_)
        if (p == child) return false;            // would create a cycle
    const math::Mat4 world = child->WorldMatrix();
    if (child->parent_) {
        auto& sib = child->parent_->children_;
        sib.erase(std::remove(sib.begin(), sib.end(), child), sib.end());
    }
    child->parent_ = parent;
    if (parent) parent->children_.push_back(child);
    SetWorldMatrix(child, world);                // stay where it was in the world
    return true;
}

void Scene::DeleteNode(Node* node) {
    if (!node) return;
    const std::vector<Node*> kids = node->children_;
    for (Node* c : kids) Link(c, node->parent_);
    Link(node, nullptr);
    nodes_.erase(std::remove_if(nodes_.begin(), nodes_.end(),
                                [node](const std::unique_ptr<Node>& p) { return p.get() == node; }),
                 nodes_.end());
}

} // namespace scene
