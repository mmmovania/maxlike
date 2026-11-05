#include "scene/Modifier.h"

namespace scene {

bool EditMeshModifier::Apply(Mesh& mesh) const {
    if (mesh.verts.size() != expected_) return false;   // indices would point elsewhere
    for (const Move& m : moves_)
        if (m.vertex < mesh.verts.size()) mesh.verts[m.vertex] = mesh.verts[m.vertex] + m.delta;
    return true;
}

Mesh DerivedObject::Evaluate() const {
    skipped_.clear();
    Mesh mesh = base_ ? base_->Evaluate() : Mesh{};
    for (const auto& mod : modifiers_) {
        if (!mod || !mod->Enabled()) continue;
        if (!mod->Apply(mesh)) skipped_.push_back(mod->ClassName());
    }
    return mesh;
}

} // namespace scene
