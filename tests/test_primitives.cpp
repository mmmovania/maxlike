// Unit tests for Box, Plane and Pyramid, against the verified results of the JS
// reader (docs/reference/max-reader-handoff.md) and Box's verified vertex and face
// order (docs/reference/max-format.md §7.2).
#include "scene/Primitives.h"
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <map>
#include <utility>

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

// Closed, consistently oriented surface: every directed edge appears exactly once and
// its reverse exactly once (so each edge joins exactly two faces, wound oppositely).
static bool Watertight(const Mesh& m) {
    std::map<std::pair<uint32_t, uint32_t>, int> directed;
    for (const Face& f : m.faces)
        for (int k = 0; k < 3; ++k) directed[{f.v[k], f.v[(k + 1) % 3]}]++;
    for (const auto& [e, n] : directed) {
        if (n != 1) return false;
        auto rev = directed.find({e.second, e.first});
        if (rev == directed.end() || rev->second != 1) return false;
    }
    return true;
}

// Faces point away from the Z axis on the sides (towards it on a tube's inner wall)
// and up/down on the caps.
static bool RevolvedFacesOutward(const Mesh& m, double innerRadius = -1) {
    for (size_t i = 0; i < m.faces.size(); ++i) {
        const Face& f = m.faces[i];
        const math::Vec3 c = (m.verts[f.v[0]] + m.verts[f.v[1]] + m.verts[f.v[2]]) * (1.0 / 3);
        const math::Vec3 n = m.FaceNormal(i);
        if (math::Length(math::Cross(m.verts[f.v[1]] - m.verts[f.v[0]], m.verts[f.v[2]] - m.verts[f.v[0]])) < 1e-12)
            continue;                                          // zero-area (a cone's tip): no direction
        if (std::fabs(n.z) > 0.99) continue;                   // caps: checked by watertightness
        const double radial = n.x * c.x + n.y * c.y;
        const bool inner = innerRadius > 0 && std::hypot(c.x, c.y) < innerRadius + 1e-6;
        if (inner ? radial >= 0 : radial <= 0) return false;
    }
    return true;
}

static void Cylinder() {
    CylinderObject c(15, 25);                                  // defaults: 5 height segs, 1 cap seg, 18 sides
    const Mesh m = c.Evaluate();
    const int S = 18, H = 5;
    CHECK(m.faces.size() == static_cast<size_t>(2 * S * H + 2 * S));
    CHECK(m.verts.size() == static_cast<size_t>(S * (H + 1) + 2));
    CHECK(Watertight(m) && RevolvedFacesOutward(m));
    CHECK(std::fabs(m.GetBounds().max.z - 25) < 1e-9 && std::fabs(m.GetBounds().max.x - 15) < 1e-9);
    // Cap segments add rings: 2 cap segs -> each cap gains S quads (2S triangles).
    c.Params().Set(CylinderObject::kCapSegs, 2);
    const Mesh m2 = c.Evaluate();
    CHECK(m2.faces.size() == static_cast<size_t>(2 * S * H + 2 * (S + 2 * S)));
    CHECK(Watertight(m2));
    // R3.1: 110 vertices, 216 faces (File > Summary Info of cylinder.max: 18 sides, 5 height segs).
    CHECK(m.verts.size() == 110 && m.faces.size() == 216);
    // Visible edges: the rings and the vertical lines; no cap spokes (R3.1 wireframe).
    CHECK(m.VisibleEdges().size() == static_cast<size_t>(S * (H + 1) + S * H));
}

static void Cone() {
    // cone.max: radius 1 = 10, radius 2 stored -20 (clamped to 0), height 20, 24 sides,
    // 5 height segs. R3.1 Summary Info: 146 vertices, 288 faces: the tip keeps a full ring
    // of 24 vertices and a zero-size top cap (the cylinder's formulas).
    ConeObject c(10, -20, 20);
    const Mesh m = c.Evaluate();
    const int S = 24, H = 5;
    CHECK(m.verts.size() == 146 && m.faces.size() == 288);
    CHECK(m.verts.size() == static_cast<size_t>(S * (H + 1) + 2) && m.faces.size() == static_cast<size_t>(2 * S * H + 2 * S));
    CHECK(Watertight(m) && RevolvedFacesOutward(m));
    CHECK(std::fabs(m.GetBounds().max.z - 20) < 1e-9 && std::fabs(m.GetBounds().max.x - 10) < 1e-9);
    // A frustum (both radii > 0) has both caps.
    ConeObject f(10, 5, 20);
    CHECK(f.Evaluate().faces.size() == static_cast<size_t>(2 * S * H + 2 * S) && Watertight(f.Evaluate()));
    // Negative height builds downwards, still closed and outward.
    ConeObject d(10, 4, -20);
    const Mesh md = d.Evaluate();
    CHECK(std::fabs(md.GetBounds().min.z + 20) < 1e-9 && std::fabs(md.GetBounds().max.z) < 1e-9);
    CHECK(Watertight(md) && RevolvedFacesOutward(md));
}

static void Tube() {
    // tube.max: outer 10, inner 6.185, height 10, 18 sides, 5 height segs, 1 cap seg.
    // R3.1 Summary Info: 216 vertices, 432 faces.
    TubeObject t(10, 6.185, 10);
    const Mesh m = t.Evaluate();
    CHECK(m.faces.size() == 432 && m.verts.size() == 216);
    CHECK(m.verts.size() == static_cast<size_t>(2 * 18 * 6));
    CHECK(Watertight(m) && RevolvedFacesOutward(m, 6.185));
    // Swapped radii give the same tube.
    CHECK(TubeObject(6.185, 10, 10).Evaluate().faces.size() == 432);
}

static void Torus() {
    TorusObject t(25, 5);                                      // 24 segments x 12 sides
    const Mesh m = t.Evaluate();
    // R3.1 Summary Info of torus.max: 288 vertices, 576 faces.
    CHECK(m.faces.size() == 576 && m.verts.size() == 288);
    CHECK(Watertight(m));
    // Every face points away from the ring's centre circle.
    bool outward = true;
    for (size_t i = 0; i < m.faces.size(); ++i) {
        const Face& f = m.faces[i];
        const math::Vec3 c = (m.verts[f.v[0]] + m.verts[f.v[1]] + m.verts[f.v[2]]) * (1.0 / 3);
        const double d = std::hypot(c.x, c.y);
        const math::Vec3 onCircle{c.x / d * 25, c.y / d * 25, 0};
        if (math::Dot(m.FaceNormal(i), c - onCircle) <= 0) outward = false;
    }
    CHECK(outward);
    CHECK(std::fabs(m.GetBounds().max.x - 30) < 1e-9 && std::fabs(m.GetBounds().max.z - 5) < 0.2);
}

static void ClosedBoxAndPyramid() {
    BoxObject b(30, 20, 10);
    b.Params().Set(BoxObject::kWidthSegs, 3);
    b.Params().Set(BoxObject::kHeightSegs, 4);
    CHECK(Watertight(BoxObject(60, 60, 60).Evaluate()) && Watertight(b.Evaluate()));
    CHECK(Watertight(BoxObject(10, 10, -20).Evaluate()));
    PyramidObject p(20, 30, 15);
    p.Params().Set(PyramidObject::kWidthSegs, 2);
    p.Params().Set(PyramidObject::kHeightSegs, 3);
    CHECK(Watertight(PyramidObject(20, 20, 10).Evaluate()) && Watertight(p.Evaluate()));
}

static bool OnSphere(const Mesh& m, double r, double lift = 0) {
    for (const math::Vec3& v : m.verts)
        if (std::fabs(math::Length(v - math::Vec3{0, 0, lift}) - r) > 1e-9) return false;
    return true;
}

static void Sphere() {
    // sphere.max: radius 10, 32 segments. R3.1 Summary Info: 482 vertices, 960 faces.
    const Mesh m = SphereObject(10).Evaluate();
    CHECK(m.verts.size() == 482 && m.faces.size() == 960);
    CHECK(Watertight(m) && FacesOutward(m) && OnSphere(m, 10));
    CHECK(std::fabs(m.verts.front().z - 10) < 1e-12);        // a vertex on each pole
    // Visible edges: 32 meridians of 16 segments and 15 rings of 32 (no hidden diagonals).
    CHECK(m.VisibleEdges().size() == 32u * 16 + 15u * 32);
    SphereObject b(10);
    b.Params().Set(SphereObject::kBaseToPivot, 1);
    const Bounds bb = b.Evaluate().GetBounds();
    CHECK(std::fabs(bb.min.z) < 1e-12 && std::fabs(bb.max.z - 20) < 1e-12);
}

static void GeoSphere() {
    // geosphere.max: radius 10, icosahedron, 4 segments. R3.1: 162 vertices, 320 faces.
    const Mesh m = GeoSphereObject(10).Evaluate();
    CHECK(m.verts.size() == 162 && m.faces.size() == 320);
    CHECK(Watertight(m) && FacesOutward(m) && OnSphere(m, 10));
    bool pole = false;
    for (const math::Vec3& v : m.verts) pole = pole || (std::fabs(v.x) < 1e-9 && std::fabs(v.y) < 1e-9 && v.z > 9.999);
    CHECK(pole);                                              // a vertex on the +Z pole
    // Other base solids and frequencies: 4 n^2 / 8 n^2 / 20 n^2 faces, 2 n^2 + 2 / 4 n^2 + 2 /
    // 10 n^2 + 2 vertices.
    for (int type = 0; type < 3; ++type)
        for (int n : {1, 2, 3, 5}) {
            GeoSphereObject g(5);
            g.Params().Set(GeoSphereObject::kBaseType, type);
            g.Params().Set(GeoSphereObject::kSegments, n);
            const Mesh gm = g.Evaluate();
            const size_t base = type == 0 ? 4 : type == 1 ? 8 : 20;
            CHECK(gm.faces.size() == base * static_cast<size_t>(n * n));
            CHECK(gm.verts.size() == base / 2 * static_cast<size_t>(n * n) + 2);
            CHECK(Watertight(gm) && FacesOutward(gm));
        }
}

int main() {
    Sphere();
    GeoSphere();
    ClosedBoxAndPyramid();
    Cylinder();
    Cone();
    Tube();
    Torus();
    BoxMatchesMaxOrder();
    SegmentedBox();
    GridEdgeCount();
    NegativeHeightBuildsDown();
    Pyramid();
    if (failures == 0) std::printf("primitives: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
