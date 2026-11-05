// Unit tests for Box, Plane and Pyramid, against the verified results of the JS
// reader (docs/reference/max-reader-handoff.md) and Box's verified vertex and face
// order (docs/reference/max-format.md §7.2).
#include "scene/Primitives.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(const math::Vec3& a, const math::Vec3& b) {
    return std::fabs(a.x - b.x) < 1e-9 && std::fabs(a.y - b.y) < 1e-9 && std::fabs(a.z - b.z) < 1e-9;
}

using namespace scene;

// Every face points away from the centre of the mesh's bounding box (convex shapes).
static bool FacesOutward(const Mesh& m) {
    const Bounds b = m.GetBounds();
    const math::Vec3 centre = (b.min + b.max) * 0.5;
    for (size_t i = 0; i < m.faces.size(); ++i) {
        const Face& f = m.faces[i];
        const math::Vec3 c = (m.verts[f.v[0]] + m.verts[f.v[1]] + m.verts[f.v[2]]) * (1.0 / 3);
        if (math::Dot(m.FaceNormal(i), c - centre) <= 0) return false;
    }
    return true;
}

static void BoxMatchesMaxOrder() {
    // box.max: Box 60 x 60 x 60, 12 triangles, base at z = 0.
    const Mesh m = BoxObject(60, 60, 60).Evaluate();
    CHECK(m.verts.size() == 8 && m.faces.size() == 12 && m.Valid());
    const math::Vec3 expected[8] = {{-30, -30, 0}, {30, -30, 0}, {-30, 30, 0}, {30, 30, 0},
                                    {-30, -30, 60}, {30, -30, 60}, {-30, 30, 60}, {30, 30, 60}};
    for (int i = 0; i < 8; ++i) CHECK(Near(m.verts[static_cast<size_t>(i)], expected[i]));
    // Faces exactly as Max lists them: bottom, top, front, right, back, left.
    const uint32_t faces[12][3] = {{0, 2, 3}, {3, 1, 0}, {4, 5, 7}, {7, 6, 4}, {0, 1, 5}, {5, 4, 0},
                                   {1, 3, 7}, {7, 5, 1}, {3, 2, 6}, {6, 7, 3}, {2, 0, 4}, {4, 6, 2}};
    for (int i = 0; i < 12; ++i)
        for (int k = 0; k < 3; ++k) CHECK(m.faces[static_cast<size_t>(i)].v[k] == faces[i][k]);
    CHECK(FacesOutward(m));
    CHECK(m.VisibleEdges().size() == 12);                  // a cube's 12 edges, no diagonals
    // Each side its own smoothing group (two faces each).
    for (int s = 0; s < 6; ++s) {
        CHECK(m.faces[static_cast<size_t>(2 * s)].smoothing == m.faces[static_cast<size_t>(2 * s + 1)].smoothing);
        if (s > 0) CHECK(m.faces[static_cast<size_t>(2 * s)].smoothing != m.faces[static_cast<size_t>(2 * s - 2)].smoothing);
    }
}

static void SegmentedBox() {
    BoxObject b(30, 20, 10);
    b.Params().Set(BoxObject::kWidthSegs, 3);
    b.Params().Set(BoxObject::kLengthSegs, 2);
    b.Params().Set(BoxObject::kHeightSegs, 4);
    const Mesh m = b.Evaluate();
    // Surface lattice points: all of 4x3x5 minus the interior 2x1x3.
    CHECK(m.verts.size() == 4 * 3 * 5 - 2 * 1 * 3);
    CHECK(m.faces.size() == 2u * 2 * (3 * 2 + 3 * 4 + 2 * 4));
    CHECK(m.Valid() && FacesOutward(m));
    CHECK(Near(m.GetBounds().Size(), {20, 30, 10}));        // width is X, length is Y
    // Visible edges = every lattice segment lying on the surface. A segment along X
    // is on the surface when its row j or layer k is on the boundary, and so on.
    const int nx = 3, ny = 2, nz = 4;
    const size_t expected = static_cast<size_t>(nx * ((ny + 1) * (nz + 1) - (ny - 1) * (nz - 1))
                                              + ny * ((nx + 1) * (nz + 1) - (nx - 1) * (nz - 1))
                                              + nz * ((nx + 1) * (ny + 1) - (nx - 1) * (ny - 1)));
    CHECK(expected == 104);
    CHECK(m.VisibleEdges().size() == expected);
}

static void GridEdgeCount() {
    // A plane with nx x ny cells has (nx+1)*ny + (ny+1)*nx visible edges.
    PlaneObject p(42, 40);           // plane.max: 40 (X) x 42 (Y), 4 x 4 segments, 32 triangles
    const Mesh m = p.Evaluate();
    CHECK(m.faces.size() == 32 && m.verts.size() == 25 && m.Valid());
    CHECK(Near(m.GetBounds().Size(), {40, 42, 0}));
    CHECK(m.VisibleEdges().size() == 5 * 4 + 5 * 4);
    for (size_t i = 0; i < m.faces.size(); ++i) CHECK(m.FaceNormal(i).z > 0.999);  // facing +Z
}

static void NegativeHeightBuildsDown() {
    const Mesh m = BoxObject(10, 10, -20).Evaluate();
    CHECK(Near(m.GetBounds().min, {-5, -5, -20}) && Near(m.GetBounds().max, {5, 5, 0}));
    CHECK(FacesOutward(m));
    CHECK(m.VisibleEdges().size() == 12);                  // flipping keeps the diagonals hidden
}

static void Pyramid() {
    // pyramid.max: 20 x 20 x 10, 6 triangles.
    const Mesh m = PyramidObject(20, 20, 10).Evaluate();
    CHECK(m.faces.size() == 6 && m.verts.size() == 5 && m.Valid());
    CHECK(Near(m.GetBounds().min, {-10, -10, 0}) && Near(m.GetBounds().max, {10, 10, 10}));
    CHECK(FacesOutward(m));
    CHECK(m.VisibleEdges().size() == 8);                   // 4 base + 4 slanted edges
    // Segmented: 2 x 3 x 2 -> sides 2*(2+3)*... cells; check counts and orientation.
    PyramidObject s(20, 30, 15);
    s.Params().Set(PyramidObject::kWidthSegs, 2);
    s.Params().Set(PyramidObject::kDepthSegs, 3);
    s.Params().Set(PyramidObject::kHeightSegs, 2);
    const Mesh n = s.Evaluate();
    // Sides: (2+3+2+3) columns x (1 quad row = 2 tris + 1 apex row = 1 tri) ; base 2*3 cells x 2.
    CHECK(n.faces.size() == 10u * (2 + 1) + 2u * 3 * 2);
    CHECK(n.Valid() && FacesOutward(n));
}

int main() {
    BoxMatchesMaxOrder();
    SegmentedBox();
    GridEdgeCount();
    NegativeHeightBuildsDown();
    Pyramid();
    if (failures == 0) std::printf("primitives: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
