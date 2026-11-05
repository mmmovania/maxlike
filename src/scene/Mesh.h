#pragma once
#include "math/Mat4.h"
#include <cstdint>
#include <vector>

// A triangle mesh as 3ds Max keeps it: shared vertices, and per-face smoothing
// groups, material ID and edge visibility.
namespace scene {

struct Face {
    uint32_t v[3] = {0, 0, 0};    // vertex indices, counter-clockwise seen from outside
    uint32_t smoothing = 0;        // smoothing-group bits; 0 = flat
    uint16_t materialId = 0;
    uint8_t  edgeVisible = 0x7;    // bit k: edge from v[k] to v[(k+1)%3] is drawn in wireframe

    bool EdgeVisible(int k) const { return (edgeVisible >> k) & 1; }
};

struct Bounds {
    math::Vec3 min{1e300, 1e300, 1e300}, max{-1e300, -1e300, -1e300};
    bool Empty() const { return min.x > max.x; }
    void Add(const math::Vec3& p);
    math::Vec3 Size() const { return Empty() ? math::Vec3{} : max - min; }
};

struct Mesh {
    std::vector<math::Vec3> verts;
    std::vector<Face> faces;

    size_t FaceCount() const { return faces.size(); }
    Bounds GetBounds() const;
    math::Vec3 FaceNormal(size_t face) const;    // unit length, from the winding
    // Every visible edge once (a, b with a < b), for wireframe display.
    std::vector<std::pair<uint32_t, uint32_t>> VisibleEdges() const;
    // Checks indices are in range; returns false if not.
    bool Valid() const;
};

} // namespace scene
