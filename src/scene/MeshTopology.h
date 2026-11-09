#pragma once
#include "scene/Mesh.h"
#include <cstdint>
#include <vector>

// Edit Mesh's Polygon and Element levels and its edges (Phase 9.5). In R3 these select
// faces: a polygon is the faces joined across hidden edges (a box side's two triangles
// share a hidden diagonal), an element the faces joined by shared vertices.
namespace scene {

// The faces of the polygon / element containing `face`, sorted.
std::vector<uint32_t> PolygonOf(const Mesh& mesh, uint32_t face);
std::vector<uint32_t> ElementOf(const Mesh& mesh, uint32_t face);
// Expanded to whole polygons / elements, sorted and unique.
std::vector<uint32_t> ExpandToPolygons(const Mesh& mesh, const std::vector<uint32_t>& faces);
std::vector<uint32_t> ExpandToElements(const Mesh& mesh, const std::vector<uint32_t>& faces);

// Edges are face * 3 + side (side k runs from v[k] to v[(k + 1) % 3]).
inline uint32_t EdgeId(uint32_t face, int side) { return face * 3 + static_cast<uint32_t>(side); }
// The two vertices of an edge (false if out of range).
bool EdgeVertices(const Mesh& mesh, uint32_t edge, uint32_t& a, uint32_t& b);
// Every face that has the edge's two vertices as one of its sides (both sides of a shared edge).
std::vector<uint32_t> FacesOnEdge(const Mesh& mesh, uint32_t edge);

} // namespace scene
