#include "scene/Summary.h"

namespace scene {

Summary Summarize(const Scene& scene) {
    Summary s;
    for (const auto& n : scene.Nodes()) {
        if (!n->GetObject()) continue;
        const Mesh m = n->GetObject()->Evaluate();
        s.rows.push_back({n->Name(), n->GetObject()->ClassName(), m.verts.size(), m.FaceCount()});
        s.verts += m.verts.size();
        s.faces += m.FaceCount();
        ++s.objects;
    }
    s.total = s.objects;
    return s;
}

} // namespace scene
