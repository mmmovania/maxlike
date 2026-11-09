#include "scene/MeshTopology.h"
#include <algorithm>
#include <map>

namespace scene {

namespace {

using Key = std::pair<uint32_t, uint32_t>;
Key EdgeKey(uint32_t a, uint32_t b) { return a < b ? Key{a, b} : Key{b, a}; }

// For each undirected edge, the faces (and sides) that have it.
std::map<Key, std::vector<std::pair<uint32_t, int>>> EdgeFaces(const Mesh& m) {
    std::map<Key, std::vector<std::pair<uint32_t, int>>> out;
    for (uint32_t f = 0; f < m.faces.size(); ++f)
        for (int k = 0; k < 3; ++k) out[EdgeKey(m.faces[f].v[k], m.faces[f].v[(k + 1) % 3])].push_back({f, k});
    return out;
}

// Faces reachable from `face` by steps `joined(face, side, neighbour face)` allows.
template <typename Joined>
std::vector<uint32_t> Flood(const Mesh& m, uint32_t face, Joined joined) {
    std::vector<uint32_t> out;
    if (face >= m.faces.size()) return out;
    const auto edges = EdgeFaces(m);
    std::vector<char> seen(m.faces.size(), 0);
    std::vector<uint32_t> todo{face};
    seen[face] = 1;
    while (!todo.empty()) {
        const uint32_t f = todo.back();
        todo.pop_back();
        out.push_back(f);
        for (int k = 0; k < 3; ++k) {
            const auto& list = edges.at(EdgeKey(m.faces[f].v[k], m.faces[f].v[(k + 1) % 3]));
            for (const auto& [g, side] : list)
                if (!seen[g] && joined(f, k, g, side)) { seen[g] = 1; todo.push_back(g); }
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace

std::vector<uint32_t> PolygonOf(const Mesh& m, uint32_t face) {
    // Across an edge hidden in both faces.
    return Flood(m, face, [&](uint32_t f, int k, uint32_t g, int side) {
        return !(m.faces[f].edgeVisible & (1u << k)) && !(m.faces[g].edgeVisible & (1u << side));
    });
}

std::vector<uint32_t> ElementOf(const Mesh& m, uint32_t face) {
    // Faces sharing any vertex: flood over vertices.
    std::vector<uint32_t> out;
    if (face >= m.faces.size()) return out;
    std::vector<std::vector<uint32_t>> facesOf(m.verts.size());
    for (uint32_t f = 0; f < m.faces.size(); ++f)
        for (uint32_t v : m.faces[f].v) if (v < facesOf.size()) facesOf[v].push_back(f);
    std::vector<char> seen(m.faces.size(), 0);
    std::vector<uint32_t> todo{face};
    seen[face] = 1;
    while (!todo.empty()) {
        const uint32_t f = todo.back();
        todo.pop_back();
        out.push_back(f);
        for (uint32_t v : m.faces[f].v)
            if (v < facesOf.size())
                for (uint32_t g : facesOf[v]) if (!seen[g]) { seen[g] = 1; todo.push_back(g); }
    }
    std::sort(out.begin(), out.end());
    return out;
}

static std::vector<uint32_t> Expand(const Mesh& m, const std::vector<uint32_t>& faces, std::vector<uint32_t> (*of)(const Mesh&, uint32_t)) {
    std::vector<char> in(m.faces.size(), 0);
    for (uint32_t f : faces)
        if (f < m.faces.size() && !in[f])
            for (uint32_t g : of(m, f)) in[g] = 1;
    std::vector<uint32_t> out;
    for (uint32_t f = 0; f < in.size(); ++f) if (in[f]) out.push_back(f);
    return out;
}
std::vector<uint32_t> ExpandToPolygons(const Mesh& m, const std::vector<uint32_t>& faces) { return Expand(m, faces, PolygonOf); }
std::vector<uint32_t> ExpandToElements(const Mesh& m, const std::vector<uint32_t>& faces) { return Expand(m, faces, ElementOf); }

bool EdgeVertices(const Mesh& m, uint32_t edge, uint32_t& a, uint32_t& b) {
    const uint32_t f = edge / 3;
    const int k = static_cast<int>(edge % 3);
    if (f >= m.faces.size()) return false;
    a = m.faces[f].v[k];
    b = m.faces[f].v[(k + 1) % 3];
    return true;
}

std::vector<uint32_t> FacesOnEdge(const Mesh& m, uint32_t edge) {
    uint32_t a, b;
    std::vector<uint32_t> out;
    if (!EdgeVertices(m, edge, a, b)) return out;
    const Key key = EdgeKey(a, b);
    for (uint32_t f = 0; f < m.faces.size(); ++f)
        for (int k = 0; k < 3; ++k)
            if (EdgeKey(m.faces[f].v[k], m.faces[f].v[(k + 1) % 3]) == key) { out.push_back(f); break; }
    return out;
}

} // namespace scene
