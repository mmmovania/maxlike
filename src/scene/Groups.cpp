#include "scene/Groups.h"
#include <algorithm>
#include <functional>

namespace scene::groups {

namespace {
bool Contains(const std::vector<Node::Id>& v, Node::Id id) { return std::find(v.begin(), v.end(), id) != v.end(); }

void Descendants(const Node& n, std::vector<Node::Id>& out) {
    for (const Node* c : n.Children()) { out.push_back(c->GetId()); Descendants(*c, out); }
}
} // namespace

Node* SelectableFor(Node* node) {
    Node* result = node;
    for (Node* p = node ? node->Parent() : nullptr; p; p = p->Parent())
        if (p->IsGroupHead() && !p->GroupOpen()) result = p;          // the outermost closed group wins
    return result;
}

std::vector<Node::Id> Selectable(Scene& scene, const std::vector<Node::Id>& ids) {
    std::vector<Node::Id> out;
    for (Node::Id id : ids)
        if (Node* n = scene.FindById(id)) {
            const Node::Id s = SelectableFor(n)->GetId();
            if (!Contains(out, s)) out.push_back(s);
        }
    return out;
}

std::vector<Node::Id> WithMembers(const Scene& scene, const std::vector<Node::Id>& ids) {
    std::vector<Node::Id> out;
    for (Node::Id id : ids) {
        const Node* n = scene.FindById(id);
        if (!n) continue;
        if (!Contains(out, id)) out.push_back(id);
        if (n->IsGroupHead()) {
            std::vector<Node::Id> d;
            Descendants(*n, d);
            for (Node::Id m : d) if (!Contains(out, m)) out.push_back(m);
        }
    }
    return out;
}

bool ShownSelected(const Scene& scene, const Node& node) {
    if (scene.IsSelected(node.GetId())) return true;
    for (const Node* p = node.Parent(); p; p = p->Parent())
        if (p->IsGroupHead() && scene.IsSelected(p->GetId())) return true;
    return false;
}

std::vector<Node::Id> AllSelectable(Scene& scene) {
    std::vector<Node::Id> out;
    for (const auto& n : scene.Nodes())
        if (SelectableFor(n.get()) == n.get()) out.push_back(n->GetId());
    return out;
}

bool Bounds(const Node& head, math::Vec3& lo, math::Vec3& hi) {
    lo = {1e300, 1e300, 1e300};
    hi = {-1e300, -1e300, -1e300};
    bool any = false;
    std::function<void(const Node&)> visit = [&](const Node& n) {
        if (n.GetObject()) {
            const math::Mat4 m = n.ObjectMatrix();
            for (const auto& v : n.GetObject()->Evaluate().verts) {
                const math::Vec3 w = math::TransformPoint(m, v);
                lo = {std::min(lo.x, w.x), std::min(lo.y, w.y), std::min(lo.z, w.z)};
                hi = {std::max(hi.x, w.x), std::max(hi.y, w.y), std::max(hi.z, w.z)};
                any = true;
            }
        }
        for (const Node* c : n.Children()) visit(*c);
    };
    visit(head);
    return any;
}

std::unique_ptr<MacroCommand> MakeGroup(Scene& scene, const std::vector<Node::Id>& members, const std::string& name) {
    auto macro = std::make_unique<MacroCommand>("Group");
    // The topmost members only: a selected child stays under its selected parent.
    std::vector<Node*> top;
    for (Node::Id id : members) {
        Node* n = scene.FindById(id);
        if (!n) continue;
        bool under = false;
        for (const Node* p = n->Parent(); p && !under; p = p->Parent()) under = Contains(members, p->GetId());
        if (!under) top.push_back(n);
    }
    if (top.empty()) return nullptr;
    Node* head = scene.CreateNode(name);
    head->SetGroup(true, false);
    // At the members' centre (their bounding box, else their pivots).
    math::Vec3 lo{1e300, 1e300, 1e300}, hi{-1e300, -1e300, -1e300}, sum{0, 0, 0};
    bool any = false;
    for (Node* n : top) {
        math::Vec3 a, b;
        sum = sum + math::TransformPoint(n->WorldMatrix(), {0, 0, 0});
        if (n->GetObject() || n->IsGroupHead()) {
            // A temporary head-like view: Bounds of the node itself and its children.
            if (Bounds(*n, a, b)) {
                lo = {std::min(lo.x, a.x), std::min(lo.y, a.y), std::min(lo.z, a.z)};
                hi = {std::max(hi.x, b.x), std::max(hi.y, b.y), std::max(hi.z, b.z)};
                any = true;
            }
        }
    }
    head->Transform().position = any ? (lo + hi) * 0.5 : sum * (1.0 / static_cast<double>(top.size()));
    macro->Add(std::make_unique<AddedNodesCommand>("Group", std::vector<Node::Id>{head->GetId()}));
    for (Node* n : top) macro->Add(std::make_unique<LinkCommand>(n->GetId(), head->GetId()));
    macro->Add(std::make_unique<SelectCommand>(std::vector<Node::Id>{head->GetId()}));
    return macro;
}

std::unique_ptr<MacroCommand> MakeUngroup(Scene& scene, const std::vector<Node::Id>& heads, bool explode) {
    auto macro = std::make_unique<MacroCommand>(explode ? "Explode" : "Ungroup");
    std::vector<Node::Id> freed;
    // Explode: every group head inside the chosen ones too, innermost first.
    std::vector<Node*> list;
    std::function<void(Node*)> collect = [&](Node* h) {
        if (explode)
            for (Node* c : h->Children()) if (c->IsGroupHead()) collect(c);
        list.push_back(h);
    };
    for (Node::Id id : heads)
        if (Node* h = scene.FindById(id); h && h->IsGroupHead()) collect(h);
    if (list.empty()) return nullptr;
    // The members are selected first: undo runs backwards, so the old selection (the
    // heads) is restored only after the heads exist again.
    std::vector<Node::Id> members;
    for (Node* h : list)
        for (Node* c : h->Children()) if (!(explode && c->IsGroupHead())) members.push_back(c->GetId());
    macro->Add(std::make_unique<SelectCommand>(members));
    for (Node* h : list) {
        // The outermost dissolved group's parent receives everything (Explode); a plain
        // ungroup hands the members to the head's own parent.
        Node* target = h->Parent();
        while (explode && target && std::find(list.begin(), list.end(), target) != list.end()) target = target->Parent();
        const Node::Id targetId = target ? target->GetId() : 0;
        const std::vector<Node*> children = h->Children();
        for (Node* c : children) {
            if (explode && c->IsGroupHead()) continue;               // handled as a head of its own
            macro->Add(std::make_unique<LinkCommand>(c->GetId(), targetId));
            freed.push_back(c->GetId());
        }
    }
    for (Node* h : list) macro->Add(std::make_unique<DeleteNodeCommand>(h->GetId()));
    (void)freed;
    return macro;
}

std::unique_ptr<MacroCommand> MakeDetach(Scene& scene, const std::vector<Node::Id>& members) {
    auto macro = std::make_unique<MacroCommand>("Detach");
    bool any = false;
    for (Node::Id id : members) {
        Node* n = scene.FindById(id);
        if (!n || !n->Parent() || !n->Parent()->IsGroupHead()) continue;
        Node* outside = n->Parent()->Parent();
        macro->Add(std::make_unique<LinkCommand>(id, outside ? outside->GetId() : 0));
        any = true;
    }
    return any ? std::move(macro) : nullptr;
}

std::unique_ptr<MacroCommand> MakeAttach(Scene& scene, const std::vector<Node::Id>& nodes, Node::Id head) {
    Node* h = scene.FindById(head);
    if (!h || !h->IsGroupHead()) return nullptr;
    auto macro = std::make_unique<MacroCommand>("Attach");
    bool any = false;
    for (Node::Id id : nodes) {
        Node* n = scene.FindById(id);
        if (!n || n == h) continue;
        bool cycle = false;                                          // never into its own group
        for (const Node* p = h; p && !cycle; p = p->Parent()) cycle = p == n;
        if (cycle) continue;
        macro->Add(std::make_unique<LinkCommand>(id, head));
        any = true;
    }
    if (!any) return nullptr;
    macro->Add(std::make_unique<SelectCommand>(std::vector<Node::Id>{head}));
    return macro;
}

} // namespace scene::groups
