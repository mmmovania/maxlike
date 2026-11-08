#include "scene/Clone.h"
#include "scene/CreationTool.h"
#include "scene/Modifier.h"
#include <algorithm>
#include <cctype>
#include <map>

namespace scene {

namespace {

void CopyParams(const ParamBlock& from, ParamBlock& to) {
    for (size_t i = 0; i < from.Count(); ++i)
        if (from.Has(i) && to.Has(i)) to.Set(i, from.Float(i));
}

std::shared_ptr<Modifier> CopyModifier(const std::shared_ptr<Modifier>& m) {
    std::shared_ptr<Modifier> out;
    if (auto em = std::dynamic_pointer_cast<EditMeshModifier>(m)) {
        auto copy = std::make_shared<EditMeshModifier>(em->ExpectedVertexCount(), em->Moves());
        copy->SetSelectedVertices(em->SelectedVertices());
        out = copy;
    } else if (std::dynamic_pointer_cast<BendModifier>(m)) out = std::make_shared<BendModifier>();
    else if (std::dynamic_pointer_cast<TaperModifier>(m)) out = std::make_shared<TaperModifier>();
    else if (std::dynamic_pointer_cast<TwistModifier>(m)) out = std::make_shared<TwistModifier>();
    else return m;                                          // unknown: shared
    CopyParams(m->Params(), out->Params());
    out->SetEnabled(m->Enabled());
    return out;
}

} // namespace

std::shared_ptr<Object> CopyObject(const std::shared_ptr<Object>& o) {
    if (!o) return nullptr;
    if (auto d = std::dynamic_pointer_cast<DerivedObject>(o)) {
        auto copy = std::make_shared<DerivedObject>(CopyObject(d->Base()));
        for (const auto& m : d->Modifiers()) copy->AddModifier(CopyModifier(m));
        return copy;
    }
    if (std::dynamic_pointer_cast<MeshObject>(o)) return std::make_shared<MeshObject>(o->Evaluate());
    for (int t = 0; t < 10; ++t)
        if (o->ClassName() == PrimitiveBaseName(t)) {
            auto copy = MakePrimitive(t);
            CopyParams(o->Params(), copy->Params());
            return copy;
        }
    return o;
}

std::string NameBase(const std::string& name) {
    std::string base = name;
    while (!base.empty() && std::isdigit(static_cast<unsigned char>(base.back()))) base.pop_back();
    return base.empty() ? "Object" : base;
}

std::vector<Node::Id> CloneNodes(Scene& scene, const std::vector<Node::Id>& ids, bool instance, const std::string& name) {
    // The originals, parents before children, so a clone's parent clone already exists.
    std::vector<Node*> originals;
    for (Node::Id id : ids)
        if (Node* n = scene.FindById(id)) originals.push_back(n);
    auto depth = [](const Node* n) { int d = 0; for (const Node* p = n->Parent(); p; p = p->Parent()) ++d; return d; };
    std::stable_sort(originals.begin(), originals.end(), [&](const Node* a, const Node* b) { return depth(a) < depth(b); });
    std::map<const Node*, Node*> cloneOf;
    std::map<const Object*, std::shared_ptr<Object>> copied;   // objects shared among the originals stay shared among the copies
    std::vector<Node::Id> out;
    for (Node* n : originals) {
        const std::string newName = (originals.size() == 1 && !name.empty()) ? name : scene.UniqueName(NameBase(n->Name()));
        Node* c = scene.CreateNode(newName);
        c->SetWireColor(n->WireColor());
        if (n->GetObject()) {
            if (instance) c->SetObject(n->GetObject());
            else {
                auto& slot = copied[n->GetObject().get()];
                if (!slot) slot = CopyObject(n->GetObject());
                c->SetObject(slot);
            }
        }
        // Linked while still at identity (Scene::Link keeps world positions), then the
        // original's local transform: the clone lands exactly on the original.
        Node* parent = n->Parent();
        if (parent) {
            const auto it = cloneOf.find(parent);
            scene.Link(c, it != cloneOf.end() ? it->second : parent);
        }
        c->Transform() = n->Transform();
        c->ObjectOffset() = n->ObjectOffset();
        cloneOf[n] = c;
        out.push_back(c->GetId());
    }
    return out;
}

} // namespace scene
