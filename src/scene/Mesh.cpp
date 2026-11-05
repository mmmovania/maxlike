#include "scene/Mesh.h"
#include <algorithm>
#include <set>

namespace scene {

void Bounds::Add(const math::Vec3& p) {
    min = {std::min(min.x, p.x), std::min(min.y, p.y), std::min(min.z, p.z)};
    max = {std::max(max.x, p.x), std::max(max.y, p.y), std::max(max.z, p.z)};
}

Bounds Mesh::GetBounds() const {
    Bounds b;
    for (const math::Vec3& v : verts) b.Add(v);
    return b;
}

math::Vec3 Mesh::FaceNormal(size_t i) const {
    const Face& f = faces[i];
    const math::Vec3 a = verts[f.v[0]], b = verts[f.v[1]], c = verts[f.v[2]];
    return math::Normalize(math::Cross(b - a, c - a));
}

std::vector<std::pair<uint32_t, uint32_t>> Mesh::VisibleEdges() const {
    std::set<std::pair<uint32_t, uint32_t>> edges; // shared edges appear once
    for (const Face& f : faces)
        for (int k = 0; k < 3; ++k)
            if (f.EdgeVisible(k)) {
                const uint32_t a = f.v[k], b = f.v[(k + 1) % 3];
                edges.insert({std::min(a, b), std::max(a, b)});
            }
    return {edges.begin(), edges.end()};
}

bool Mesh::Valid() const {
    for (const Face& f : faces)
        for (uint32_t i : f.v)
            if (i >= verts.size()) return false;
    return true;
}

} // namespace scene
