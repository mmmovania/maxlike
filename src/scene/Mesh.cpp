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

std::vector<std::pair<uint32_t, uint32_t>> Mesh::VisibleEdges(const std::vector<bool>* faceShown) const {
    std::set<std::pair<uint32_t, uint32_t>> edges; // shared edges appear once
    for (size_t i = 0; i < faces.size(); ++i) {
        const Face& f = faces[i];
        if (faceShown && i < faceShown->size() && !(*faceShown)[i]) continue;
        for (int k = 0; k < 3; ++k)
            if (f.EdgeVisible(k)) {
                const uint32_t a = f.v[k], b = f.v[(k + 1) % 3];
                edges.insert({std::min(a, b), std::max(a, b)});
            }
    }
    return {edges.begin(), edges.end()};
}

std::vector<math::Vec3> Mesh::CornerNormals() const {
    // Unit face normals (zero for zero-area faces) and, per vertex, the faces using it.
    std::vector<math::Vec3> fn(faces.size());
    std::vector<std::vector<uint32_t>> around(verts.size());
    for (size_t i = 0; i < faces.size(); ++i) {
        const Face& f = faces[i];
        const math::Vec3 c = math::Cross(verts[f.v[1]] - verts[f.v[0]], verts[f.v[2]] - verts[f.v[0]]);
        fn[i] = math::Length(c) > 1e-12 ? math::Normalize(c) : math::Vec3{0, 0, 0};
        for (uint32_t v : f.v) around[v].push_back(static_cast<uint32_t>(i));
    }
    std::vector<math::Vec3> out;
    out.reserve(faces.size() * 3);
    for (size_t i = 0; i < faces.size(); ++i) {
        const Face& f = faces[i];
        for (uint32_t v : f.v) {
            if (f.smoothing == 0) { out.push_back(fn[i]); continue; }
            math::Vec3 sum{0, 0, 0};
            for (uint32_t j : around[v])
                if (faces[j].smoothing & f.smoothing) sum = sum + fn[j];
            out.push_back(math::Length(sum) > 1e-12 ? math::Normalize(sum) : fn[i]);
        }
    }
    return out;
}

bool Mesh::Valid() const {
    for (const Face& f : faces)
        for (uint32_t i : f.v)
            if (i >= verts.size()) return false;
    return true;
}

} // namespace scene
