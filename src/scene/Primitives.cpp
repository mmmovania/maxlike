#include "scene/Primitives.h"
#include "scene/TeapotData.h"
#include <algorithm>
#include <array>
#include <iterator>
#include <cmath>
#include <map>
#include <tuple>

namespace scene {

namespace {

int Segs(const ParamBlock& p, size_t i) { return std::clamp(p.Int(i, 1), 1, 200); }

// Adds one cell of a grid as Max does (max-format.md §7.2): two triangles
// (c00, c10, c11) and (c11, c01, c00), each with its two outer edges visible and
// the diagonal (its third edge) hidden.
void AddCell(Mesh& m, uint32_t c00, uint32_t c10, uint32_t c11, uint32_t c01, uint32_t smoothing) {
    Face a; a.v[0] = c00; a.v[1] = c10; a.v[2] = c11; a.edgeVisible = 0x3; a.smoothing = smoothing;
    Face b; b.v[0] = c11; b.v[1] = c01; b.v[2] = c00; b.edgeVisible = 0x3; b.smoothing = smoothing;
    m.faces.push_back(a);
    m.faces.push_back(b);
}

// Reverse every face (used when an odd number of dimensions is negative, so the
// mirrored mesh still faces outwards). Edge k runs v[k] -> v[k+1], so after swapping
// v[1] and v[2] the old edges 0, 1, 2 become edges 2, 1, 0.
void Flip(Mesh& m) {
    for (Face& f : m.faces) {
        std::swap(f.v[1], f.v[2]);
        const uint8_t e = f.edgeVisible;
        f.edgeVisible = static_cast<uint8_t>(((e & 1) << 2) | (e & 2) | ((e >> 2) & 1));
    }
}

} // namespace

// ---------------------------------------------------------------- Box

BoxObject::BoxObject(double length, double width, double height) {
    params_.Define(kLength, ParamType::Float, length);
    params_.Define(kWidth, ParamType::Float, width);
    params_.Define(kHeight, ParamType::Float, height);
    params_.Define(kWidthSegs, ParamType::Int, 1);
    params_.Define(kLengthSegs, ParamType::Int, 1);
    params_.Define(kHeightSegs, ParamType::Int, 1);
}

// Vertices are the points of an (nx+1) x (ny+1) x (nz+1) lattice that lie on the
// surface, numbered layer by layer from the bottom, each layer row by row (y), each
// row by x. For a 1 x 1 x 1 box this is exactly Max's order (max-format.md §7.2):
// 0 (-x,-y,0) 1 (+x,-y,0) 2 (-x,+y,0) 3 (+x,+y,0), then 4..7 the same at the top.
// NOTE: for segmented boxes Max's own order is not verified yet.
Mesh BoxObject::Evaluate() const {
    const double l = params_.Float(kLength), w = params_.Float(kWidth), h = params_.Float(kHeight);
    const int nx = Segs(params_, kWidthSegs), ny = Segs(params_, kLengthSegs), nz = Segs(params_, kHeightSegs);

    Mesh m;
    std::map<std::tuple<int, int, int>, uint32_t> index;
    for (int k = 0; k <= nz; ++k)
        for (int j = 0; j <= ny; ++j)
            for (int i = 0; i <= nx; ++i) {
                const bool surface = k == 0 || k == nz || j == 0 || j == ny || i == 0 || i == nx;
                if (!surface) continue;
                index[{i, j, k}] = static_cast<uint32_t>(m.verts.size());
                m.verts.push_back({w * (static_cast<double>(i) / nx - 0.5),
                                   l * (static_cast<double>(j) / ny - 0.5),
                                   h * static_cast<double>(k) / nz});
            }
    auto V = [&](int i, int j, int k) { return index.at({i, j, k}); };

    // Each side: a grid stepping along U then V from its corner c00, as in Max's
    // face list (bottom 0 2 3 / 3 1 0, top 4 5 7 / 7 6 4, ...). Own smoothing group
    // per side. NOTE: Max's exact smoothing bits per side are not verified.
    struct Side { int ci, cj, ck; int ui, uj, uk; int vi, vj, vk; int nu, nv; uint32_t group; };
    const Side sides[6] = {
        {0, 0, 0,   0, 1, 0,   1, 0, 0,   ny, nx, 1u << 1},   // bottom: U +y, V +x
        {0, 0, nz,  1, 0, 0,   0, 1, 0,   nx, ny, 1u << 2},   // top:    U +x, V +y
        {0, 0, 0,   1, 0, 0,   0, 0, 1,   nx, nz, 1u << 3},   // front (-y): U +x, V +z
        {nx, 0, 0,  0, 1, 0,   0, 0, 1,   ny, nz, 1u << 4},   // right (+x): U +y, V +z
        {nx, ny, 0, -1, 0, 0,  0, 0, 1,   nx, nz, 1u << 5},   // back (+y):  U -x, V +z
        {0, ny, 0,  0, -1, 0,  0, 0, 1,   ny, nz, 1u << 6},   // left (-x):  U -y, V +z
    };
    for (const Side& s : sides)
        for (int b = 0; b < s.nv; ++b)
            for (int a = 0; a < s.nu; ++a) {
                auto at = [&](int da, int db) {
                    return V(s.ci + s.ui * da + s.vi * db, s.cj + s.uj * da + s.vj * db, s.ck + s.uk * da + s.vk * db);
                };
                AddCell(m, at(a, b), at(a + 1, b), at(a + 1, b + 1), at(a, b + 1), s.group);
            }
    // Negative dimensions mirror the box (e.g. a negative height builds downwards);
    // keep the faces pointing outwards.
    const int negatives = (l < 0) + (w < 0) + (h < 0);
    if (negatives % 2 == 1) Flip(m);
    return m;
}

// ---------------------------------------------------------------- Plane

PlaneObject::PlaneObject(double length, double width) {
    params_.Define(kLength, ParamType::Float, length);
    params_.Define(kWidth, ParamType::Float, width);
    params_.Define(kLengthSegs, ParamType::Int, 4);
    params_.Define(kWidthSegs, ParamType::Int, 4);
}

// A grid facing +Z, cells split like the box's top. NOTE: Max's vertex order for
// Plane is not verified; this numbers rows (y) then columns (x) from (-x, -y).
Mesh PlaneObject::Evaluate() const {
    const double l = params_.Float(kLength), w = params_.Float(kWidth);
    const int nx = Segs(params_, kWidthSegs), ny = Segs(params_, kLengthSegs);
    Mesh m;
    for (int j = 0; j <= ny; ++j)
        for (int i = 0; i <= nx; ++i)
            m.verts.push_back({w * (static_cast<double>(i) / nx - 0.5), l * (static_cast<double>(j) / ny - 0.5), 0});
    auto V = [&](int i, int j) { return static_cast<uint32_t>(j * (nx + 1) + i); };
    for (int j = 0; j < ny; ++j)
        for (int i = 0; i < nx; ++i)
            AddCell(m, V(i, j), V(i + 1, j), V(i + 1, j + 1), V(i, j + 1), 1);
    if ((l < 0) != (w < 0)) Flip(m);
    return m;
}

// ---------------------------------------------------------------- Pyramid

PyramidObject::PyramidObject(double width, double depth, double height) {
    params_.Define(kWidth, ParamType::Float, width);
    params_.Define(kDepth, ParamType::Float, depth);
    params_.Define(kHeight, ParamType::Float, height);
    params_.Define(kWidthSegs, ParamType::Int, 1);
    params_.Define(kDepthSegs, ParamType::Int, 1);
    params_.Define(kHeightSegs, ParamType::Int, 1);
}

// Four sides, each a grid that shrinks towards the apex, plus the base; layout and
// winding as in the verified JS reader (max-format.md §7.1). Cells that collapse at
// the apex become single triangles with all edges visible. NOTE: Max's vertex order
// for Pyramid is not verified.
Mesh PyramidObject::Evaluate() const {
    const double w = params_.Float(kWidth), d = params_.Float(kDepth), h = params_.Float(kHeight);
    const int ws = Segs(params_, kWidthSegs), ds = Segs(params_, kDepthSegs), hs = Segs(params_, kHeightSegs);
    Mesh m;
    std::map<std::tuple<long long, long long, long long>, uint32_t> index;  // share equal points
    auto vert = [&](const math::Vec3& p) {
        const auto key = std::make_tuple(std::llround(p.x * 1e6), std::llround(p.y * 1e6), std::llround(p.z * 1e6));
        auto it = index.find(key);
        if (it != index.end()) return it->second;
        const auto id = static_cast<uint32_t>(m.verts.size());
        m.verts.push_back(p);
        index.emplace(key, id);
        return id;
    };
    auto onSide = [&](double u, double v, int layer) {          // u, v in -1..1 on the base
        const double s = 1.0 - static_cast<double>(layer) / hs;  // shrink towards the apex
        return vert({u * w / 2 * s, v * d / 2 * s, h * layer / hs});
    };

    struct Edge { double u0, v0, u1, v1; int n; uint32_t group; };
    const Edge sides[4] = {{-1, -1, 1, -1, ws, 1u << 1}, {1, -1, 1, 1, ds, 1u << 2},
                           {1, 1, -1, 1, ws, 1u << 3},   {-1, 1, -1, -1, ds, 1u << 4}};
    for (const Edge& e : sides)
        for (int i = 0; i < e.n; ++i)
            for (int j = 0; j < hs; ++j) {
                auto P = [&](double t, int layer) {
                    return onSide(e.u0 + (e.u1 - e.u0) * t, e.v0 + (e.v1 - e.v0) * t, layer);
                };
                const double t0 = static_cast<double>(i) / e.n, t1 = static_cast<double>(i + 1) / e.n;
                const uint32_t a = P(t0, j), b = P(t1, j), c = P(t1, j + 1), dd = P(t0, j + 1);
                if (c == dd) {               // top row: the cell is a triangle (a, b, apex)
                    Face f; f.v[0] = a; f.v[1] = b; f.v[2] = c; f.edgeVisible = 0x7; f.smoothing = e.group;
                    m.faces.push_back(f);
                } else {
                    AddCell(m, a, b, c, dd, e.group);
                }
            }
    // Base, facing down.
    for (int i = 0; i < ws; ++i)
        for (int k = 0; k < ds; ++k) {
            const double u0 = -1 + 2.0 * i / ws, u1 = -1 + 2.0 * (i + 1) / ws;
            const double v0 = -1 + 2.0 * k / ds, v1 = -1 + 2.0 * (k + 1) / ds;
            AddCell(m, onSide(u0, v0, 0), onSide(u0, v1, 0), onSide(u1, v1, 0), onSide(u1, v0, 0), 1u << 5);
        }
    const int negatives = (w < 0) + (d < 0) + (h < 0);
    if (negatives % 2 == 1) Flip(m);
    return m;
}

// ---------------------------------------------------------------- rings
//
// Cylinder, Cone and Tube are built from rings of vertices around the Z axis.
// Verified against R3.1 (File > Summary Info and wireframe screenshots):
//   - counts: Cylinder 18 sides x 5 height segs = 110 vertices / 216 faces; a Cone with
//     radius 2 = 0 keeps a full ring of vertices at the tip and its (zero-size) top cap:
//     24 x 5 -> 146 / 288; Tube 18 x 5 -> 216 / 432;
//   - a cap of one segment is a fan whose spokes are hidden (only the rim shows);
//   - rings start at +X (fitted to the cylinder's outline in the Top view).
// NOTE: the order of vertices and faces within the mesh is not verified.

namespace {

constexpr double kPi = 3.14159265358979323846;

int Sides(const ParamBlock& p, size_t i, int fallback) { return std::clamp(p.Int(i, fallback), 3, 200); }

// One ring of `sides` vertices at height z, starting at +X, counter-clockwise seen from
// above. A ring of radius 0 still has `sides` vertices (all at the centre), as in Max.
std::vector<uint32_t> Ring(Mesh& m, double radius, double z, int sides) {
    std::vector<uint32_t> ring;
    for (int i = 0; i < sides; ++i) {
        const double a = 2.0 * kPi * i / sides;
        ring.push_back(static_cast<uint32_t>(m.verts.size()));
        m.verts.push_back({radius * std::cos(a), radius * std::sin(a), z});
    }
    return ring;
}

// The band of faces between two rings (lower -> upper), facing away from the axis
// (or towards it when `inward`); cells split like the Box's, diagonal hidden.
void Band(Mesh& m, const std::vector<uint32_t>& lo, const std::vector<uint32_t>& hi, bool inward, uint32_t group) {
    const size_t n = lo.size();
    for (size_t i = 0; i < n; ++i) {
        const size_t j = (i + 1) % n;
        uint32_t c00 = lo[i], c10 = lo[j], c11 = hi[j], c01 = hi[i];
        if (inward) { std::swap(c00, c10); std::swap(c01, c11); }
        AddCell(m, c00, c10, c11, c01, group);
    }
}

// A flat cap from an outer ring inwards: (capSegs - 1) more rings of quads, then either
// a fan to one centre vertex (spokes hidden) or, for an annulus, quads to `inner`.
void Cap(Mesh& m, std::vector<uint32_t> outer, double rOuter, double rInner, double z, int capSegs,
         bool up, uint32_t group, const std::vector<uint32_t>* inner) {
    const int sides = static_cast<int>(outer.size());
    for (int k = 1; k <= capSegs; ++k) {
        const bool last = k == capSegs;
        if (last && !inner) {
            const auto centre = static_cast<uint32_t>(m.verts.size());
            m.verts.push_back({0, 0, z});
            for (int i = 0; i < sides; ++i) {
                uint32_t a = outer[static_cast<size_t>(i)], b = outer[static_cast<size_t>((i + 1) % sides)];
                if (!up) std::swap(a, b);
                Face f; f.v[0] = a; f.v[1] = b; f.v[2] = centre; f.smoothing = group;
                f.edgeVisible = 0x1;   // only the rim edge (a -> b) shows
                m.faces.push_back(f);
            }
            return;
        }
        const double r = rOuter + (rInner - rOuter) * k / capSegs;
        std::vector<uint32_t> next = (last && inner) ? *inner : Ring(m, r, z, sides);
        for (int i = 0; i < sides; ++i) {
            const int j = (i + 1) % sides;
            uint32_t a = outer[static_cast<size_t>(i)], b = outer[static_cast<size_t>(j)];
            uint32_t c = next[static_cast<size_t>(j)], d = next[static_cast<size_t>(i)];
            if (!up) { std::swap(a, b); std::swap(c, d); }
            AddCell(m, a, b, c, d, group);
        }
        outer = next;
    }
}

uint32_t SmoothSide(bool smooth) { return smooth ? 1u : 0u; }

// A closed solid of revolution with straight sides: Cylinder and Cone.
Mesh RingSolid(double r1, double r2, double h, int hsegs, int capSegs, int sides, bool smooth) {
    Mesh m;
    const double z0 = std::min(0.0, h), z1 = std::max(0.0, h);
    const double rb = h >= 0 ? r1 : r2, rt = h >= 0 ? r2 : r1;    // radius at the bottom / top
    std::vector<std::vector<uint32_t>> rings;
    for (int k = 0; k <= hsegs; ++k) {
        const double t = static_cast<double>(k) / hsegs;
        rings.push_back(Ring(m, rb + (rt - rb) * t, z0 + (z1 - z0) * t, sides));
    }
    for (int k = 0; k < hsegs; ++k) Band(m, rings[static_cast<size_t>(k)], rings[static_cast<size_t>(k + 1)], false, SmoothSide(smooth));
    // Both caps always exist, even with radius 0 (Max's cone keeps its zero-size top cap).
    Cap(m, rings.front(), rb, 0, z0, capSegs, false, 1u << 1, nullptr);   // bottom
    Cap(m, rings.back(), rt, 0, z1, capSegs, true, 1u << 2, nullptr);     // top
    return m;
}

} // namespace

CylinderObject::CylinderObject(double radius, double height) {
    params_.Define(kRadius, ParamType::Float, radius);
    params_.Define(kHeight, ParamType::Float, height);
    params_.Define(kHeightSegs, ParamType::Int, 5);
    params_.Define(kCapSegs, ParamType::Int, 1);
    params_.Define(kSides, ParamType::Int, 18);
    params_.Define(kSmooth, ParamType::Bool, 1);
}

Mesh CylinderObject::Evaluate() const {
    const double r = std::max(0.0, params_.Float(kRadius));
    return RingSolid(r, r, params_.Float(kHeight), Segs(params_, kHeightSegs), Segs(params_, kCapSegs),
                     Sides(params_, kSides, 18), params_.Bool(kSmooth));
}

ConeObject::ConeObject(double radius1, double radius2, double height) {
    params_.Define(kRadius1, ParamType::Float, radius1);
    params_.Define(kRadius2, ParamType::Float, radius2);
    params_.Define(kHeight, ParamType::Float, height);
    params_.Define(kHeightSegs, ParamType::Int, 5);
    params_.Define(kCapSegs, ParamType::Int, 1);
    params_.Define(kSides, ParamType::Int, 24);
    params_.Define(kSmooth, ParamType::Bool, 1);
}

Mesh ConeObject::Evaluate() const {
    // Max clamps negative radii to 0 (cone.max stores radius 2 = -20, shown as 0).
    return RingSolid(std::max(0.0, params_.Float(kRadius1)), std::max(0.0, params_.Float(kRadius2)),
                     params_.Float(kHeight), Segs(params_, kHeightSegs), Segs(params_, kCapSegs),
                     Sides(params_, kSides, 24), params_.Bool(kSmooth));
}

TubeObject::TubeObject(double radius1, double radius2, double height) {
    params_.Define(kRadius1, ParamType::Float, radius1);
    params_.Define(kRadius2, ParamType::Float, radius2);
    params_.Define(kHeight, ParamType::Float, height);
    params_.Define(kSides, ParamType::Int, 18);
    params_.Define(kCapSegs, ParamType::Int, 1);
    params_.Define(kHeightSegs, ParamType::Int, 5);
    params_.Define(kSmooth, ParamType::Bool, 1);
}

Mesh TubeObject::Evaluate() const {
    const double a = std::max(0.0, params_.Float(kRadius1)), b = std::max(0.0, params_.Float(kRadius2));
    const double ro = std::max(a, b), ri = std::min(a, b), h = params_.Float(kHeight);
    const int sides = Sides(params_, kSides, 18), hsegs = Segs(params_, kHeightSegs), capSegs = Segs(params_, kCapSegs);
    const uint32_t side = SmoothSide(params_.Bool(kSmooth));
    if (ri <= 0) return RingSolid(ro, ro, h, hsegs, capSegs, sides, params_.Bool(kSmooth)); // no hole
    Mesh m;
    const double z0 = std::min(0.0, h), z1 = std::max(0.0, h);
    std::vector<std::vector<uint32_t>> out, in;
    for (int k = 0; k <= hsegs; ++k) out.push_back(Ring(m, ro, z0 + (z1 - z0) * k / hsegs, sides));
    for (int k = 0; k <= hsegs; ++k) in.push_back(Ring(m, ri, z0 + (z1 - z0) * k / hsegs, sides));
    for (int k = 0; k < hsegs; ++k) Band(m, out[static_cast<size_t>(k)], out[static_cast<size_t>(k + 1)], false, side);
    for (int k = 0; k < hsegs; ++k) Band(m, in[static_cast<size_t>(k)], in[static_cast<size_t>(k + 1)], true, side << 1);
    Cap(m, out.front(), ro, ri, z0, capSegs, false, 1u << 2, &in.front());   // bottom ring
    Cap(m, out.back(), ro, ri, z1, capSegs, true, 1u << 3, &in.back());      // top ring
    return m;
}

// ---------------------------------------------------------------- Torus

TorusObject::TorusObject(double radius1, double radius2) {
    params_.Define(kRadius1, ParamType::Float, radius1);
    params_.Define(kRadius2, ParamType::Float, radius2);
    params_.Define(kRotation, ParamType::Float, 0);
    params_.Define(kTwist, ParamType::Float, 0);
    params_.Define(kSegments, ParamType::Int, 24);
    params_.Define(kSides, ParamType::Int, 12);
}

// segments x sides grid wrapped twice: around the ring (Z axis) and around the tube.
// Rotation turns every cross-section; twist adds a growing turn along the ring.
Mesh TorusObject::Evaluate() const {
    const double R = params_.Float(kRadius1), r = std::fabs(params_.Float(kRadius2));
    const double rot = params_.Float(kRotation) * kPi / 180, twist = params_.Float(kTwist) * kPi / 180;
    const int segs = Sides(params_, kSegments, 24), sides = Sides(params_, kSides, 12);
    Mesh m;
    for (int i = 0; i < segs; ++i) {
        const double a = 2 * kPi * i / segs;
        for (int j = 0; j < sides; ++j) {
            const double b = 2 * kPi * j / sides + rot + twist * i / segs;
            const double d = R + r * std::cos(b);
            m.verts.push_back({d * std::cos(a), d * std::sin(a), r * std::sin(b)});
        }
    }
    auto V = [&](int i, int j) { return static_cast<uint32_t>((i % segs) * sides + (j % sides)); };
    for (int i = 0; i < segs; ++i)
        for (int j = 0; j < sides; ++j)
            AddCell(m, V(i, j), V(i + 1, j), V(i + 1, j + 1), V(i, j + 1), 1);
    return m;
}

// ---------------------------------------------------------------- Sphere
//
// Verified against R3.1 (Summary Info of sphere.max, 32 segments): 482 vertices, 960
// faces = two poles plus (segments/2 - 1) latitude rings of `segments` vertices; a fan
// at each pole and quads between the rings. NOTE: the vertex order (here: top pole,
// rings from the top down starting at +X, bottom pole) is not verified.

SphereObject::SphereObject(double radius) {
    params_.Define(kRadius, ParamType::Float, radius);
    params_.Define(kSegments, ParamType::Int, 32);
    params_.Define(kSmooth, ParamType::Bool, 1);
    params_.Define(kHemisphere, ParamType::Float, 0);
    params_.Define(kChop, ParamType::Int, 0);
    params_.Define(kBaseToPivot, ParamType::Bool, 0);
}

Mesh SphereObject::Evaluate() const {
    const double r = std::fabs(params_.Float(kRadius));
    const int segs = std::clamp(params_.Int(kSegments, 32), 4, 200);
    const int rows = std::max(2, segs / 2);            // latitude bands, pole to pole
    const uint32_t group = params_.Bool(kSmooth) ? 1u : 0u;
    const double lift = params_.Bool(kBaseToPivot) ? r : 0.0;
    Mesh m;
    m.verts.push_back({0, 0, r + lift});                                   // top pole
    std::vector<std::vector<uint32_t>> rings;
    for (int k = 1; k < rows; ++k) {
        const double phi = kPi * k / rows;                                 // from the top
        std::vector<uint32_t> ring = Ring(m, r * std::sin(phi), r * std::cos(phi) + lift, segs);
        rings.push_back(std::move(ring));
    }
    const auto bottom = static_cast<uint32_t>(m.verts.size());
    m.verts.push_back({0, 0, -r + lift});                                  // bottom pole

    // Fans at the poles: meridians and the first ring are visible (no hidden edges).
    for (int i = 0; i < segs; ++i) {
        const uint32_t a = rings.front()[static_cast<size_t>(i)], b = rings.front()[static_cast<size_t>((i + 1) % segs)];
        Face f; f.v[0] = 0; f.v[1] = a; f.v[2] = b; f.edgeVisible = 0x7; f.smoothing = group;
        m.faces.push_back(f);
    }
    for (size_t k = 0; k + 1 < rings.size(); ++k)        // rings run downwards: band from lower to upper
        Band(m, rings[k + 1], rings[k], false, group);
    for (int i = 0; i < segs; ++i) {
        const uint32_t a = rings.back()[static_cast<size_t>(i)], b = rings.back()[static_cast<size_t>((i + 1) % segs)];
        Face f; f.v[0] = bottom; f.v[1] = b; f.v[2] = a; f.edgeVisible = 0x7; f.smoothing = group;
        m.faces.push_back(f);
    }
    return m;
}

// ---------------------------------------------------------------- GeoSphere
//
// Verified against R3.1 (Summary Info of geosphere.max, icosahedron, 4 segments):
// 162 vertices, 320 faces: each base triangle split into segments^2 triangles, points
// pushed onto the sphere, shared along base edges; a vertex on the +Z pole (JS reader).
// NOTE: the rotation about Z and the vertex order are not verified.

GeoSphereObject::GeoSphereObject(double radius) {
    params_.Define(kRadius, ParamType::Float, radius);
    params_.Define(kSegments, ParamType::Int, 4);
    params_.Define(kBaseType, ParamType::Int, 2);
    params_.Define(kHemisphere, ParamType::Bool, 0);
    params_.Define(kSmooth, ParamType::Bool, 1);
    params_.Define(kBaseToPivot, ParamType::Bool, 0);
}

Mesh GeoSphereObject::Evaluate() const {
    const double r = std::fabs(params_.Float(kRadius));
    const int n = std::clamp(params_.Int(kSegments, 4), 1, 100);
    const int type = std::clamp(params_.Int(kBaseType, 2), 0, 2);
    const uint32_t group = params_.Bool(kSmooth) ? 1u : 0u;
    const double lift = params_.Bool(kBaseToPivot) ? r : 0.0;

    // Base solids, each with a vertex at +Z, faces counter-clockwise from outside.
    std::vector<math::Vec3> bv;
    std::vector<std::array<int, 3>> bf;
    if (type == 1) {             // octahedron
        bv = {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0}, {0, 0, -1}};
        bf = {{0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 1}, {5, 2, 1}, {5, 3, 2}, {5, 4, 3}, {5, 1, 4}};
    } else if (type == 0) {      // tetrahedron
        const double t = std::sqrt(8.0) / 3, u = std::sqrt(2.0 / 3);
        bv = {{0, 0, 1}, {t, 0, -1.0 / 3}, {-t / 2, u, -1.0 / 3}, {-t / 2, -u, -1.0 / 3}};
        bf = {{0, 1, 2}, {0, 2, 3}, {0, 3, 1}, {1, 3, 2}};
    } else {                     // icosahedron: pole, two rings of five, pole
        const double z = 1 / std::sqrt(5.0), q = 2 / std::sqrt(5.0);
        bv.push_back({0, 0, 1});
        for (int i = 0; i < 5; ++i) bv.push_back({q * std::cos(2 * kPi * i / 5), q * std::sin(2 * kPi * i / 5), z});
        for (int i = 0; i < 5; ++i) bv.push_back({q * std::cos(2 * kPi * (i + 0.5) / 5), q * std::sin(2 * kPi * (i + 0.5) / 5), -z});
        bv.push_back({0, 0, -1});
        for (int i = 0; i < 5; ++i) {
            const int a = 1 + i, b = 1 + (i + 1) % 5, c = 6 + i, d = 6 + (i + 1) % 5;
            bf.push_back({0, a, b});
            bf.push_back({a, c, b});
            bf.push_back({b, c, d});
            bf.push_back({11, d, c});
        }
    }

    Mesh m;
    std::map<std::tuple<long long, long long, long long>, uint32_t> index;   // share points on base edges
    auto vert = [&](const math::Vec3& p) {
        const math::Vec3 s = math::Normalize(p);
        const auto key = std::make_tuple(std::llround(s.x * 1e9), std::llround(s.y * 1e9), std::llround(s.z * 1e9));
        auto it = index.find(key);
        if (it != index.end()) return it->second;
        const auto id = static_cast<uint32_t>(m.verts.size());
        m.verts.push_back({s.x * r, s.y * r, s.z * r + lift});
        index.emplace(key, id);
        return id;
    };
    for (const auto& f : bf) {
        const math::Vec3 A = bv[static_cast<size_t>(f[0])], B = bv[static_cast<size_t>(f[1])], C = bv[static_cast<size_t>(f[2])];
        // Barycentric grid: point (i, j) = A + (B - A) i/n + (C - A) j/n, i + j <= n.
        auto P = [&](int i, int j) { return vert(A + (B - A) * (static_cast<double>(i) / n) + (C - A) * (static_cast<double>(j) / n)); };
        for (int j = 0; j < n; ++j)
            for (int i = 0; i + j < n; ++i) {
                Face up; up.v[0] = P(i, j); up.v[1] = P(i + 1, j); up.v[2] = P(i, j + 1);
                up.edgeVisible = 0x7; up.smoothing = group;
                m.faces.push_back(up);
                if (i + j + 1 < n) {
                    Face dn; dn.v[0] = P(i + 1, j); dn.v[1] = P(i + 1, j + 1); dn.v[2] = P(i, j + 1);
                    dn.edgeVisible = 0x7; dn.smoothing = group;
                    m.faces.push_back(dn);
                }
            }
    }
    return m;
}

// ---------------------------------------------------------------- Teapot
//
// Verified against R3.1 (Summary Info of teapot.max, 4 segments): 530 vertices, 1024
// faces = 32 patches x 4 x 4 cells x 2 triangles. Nothing is removed (the zero-area
// triangles at the lid's knob and the base stay, as with the cone's tip), and points
// are merged only within each part (Body = rim + body + bottom, Handle, Spout, Lid):
// merging everything gives 529, because the handle's end touches the body at one
// grid point. NOTE: the vertex and face order are not verified.

TeapotObject::TeapotObject(double radius) {
    params_.Define(kRadius, ParamType::Float, radius);
    params_.Define(kSegments, ParamType::Int, 4);
    params_.Define(kSmooth, ParamType::Bool, 1);
    params_.Define(kUnused, ParamType::Int, -1);
    params_.Define(kBody, ParamType::Bool, 1);
    params_.Define(kHandle, ParamType::Bool, 1);
    params_.Define(kSpout, ParamType::Bool, 1);
    params_.Define(kLid, ParamType::Bool, 1);
}

namespace {

// Cubic Bernstein weights.
void Bernstein(double t, double b[4]) {
    const double s = 1 - t;
    b[0] = s * s * s; b[1] = 3 * t * s * s; b[2] = 3 * t * t * s; b[3] = t * t * t;
}

math::Vec3 PatchPoint(int patch, double u, double v) {
    double bu[4], bv[4];
    Bernstein(u, bu);
    Bernstein(v, bv);
    math::Vec3 p{0, 0, 0};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) {
            const double* c = teapot::kPoints[teapot::kPatches[patch][i * 4 + j]];
            const double w = bu[i] * bv[j];
            p = p + math::Vec3{c[0], c[1], c[2]} * w;
        }
    return p;
}

} // namespace

Mesh TeapotObject::Evaluate() const {
    const double radius = std::fabs(params_.Float(kRadius));
    const double scale = radius / 2.0;                       // the data's body radius is 2.0
    const int n = std::clamp(params_.Int(kSegments, 4), 1, 64);
    const uint32_t group = params_.Bool(kSmooth) ? 1u : 0u;

    struct Part { size_t param; int first, count; };
    const Part parts[] = {
        {kBody, 0, 12}, {kBody, 28, 4},                       // rim + body, bottom
        {kHandle, 12, 4}, {kSpout, 16, 4}, {kLid, 20, 8},
    };
    // Body's three pieces are one part for merging; give each part an id.
    const int partId[] = {0, 0, 1, 2, 3};

    Mesh m;
    std::map<std::tuple<int, long long, long long, long long>, uint32_t> index;
    for (size_t pi = 0; pi < std::size(parts); ++pi) {
        const Part& part = parts[pi];
        if (!params_.Bool(part.param)) continue;
        for (int patch = part.first; patch < part.first + part.count; ++patch) {
            std::vector<uint32_t> grid(static_cast<size_t>((n + 1) * (n + 1)));
            for (int a = 0; a <= n; ++a)
                for (int b = 0; b <= n; ++b) {
                    const math::Vec3 p = PatchPoint(patch, static_cast<double>(a) / n, static_cast<double>(b) / n) * scale;
                    const auto key = std::make_tuple(partId[pi], std::llround(p.x * 1e6 / scale),
                                                     std::llround(p.y * 1e6 / scale), std::llround(p.z * 1e6 / scale));
                    auto it = index.find(key);
                    uint32_t id;
                    if (it != index.end()) {
                        id = it->second;
                    } else {
                        id = static_cast<uint32_t>(m.verts.size());
                        m.verts.push_back(p);
                        index.emplace(key, id);
                    }
                    grid[static_cast<size_t>(a * (n + 1) + b)] = id;
                }
            auto G = [&](int a, int b) { return grid[static_cast<size_t>(a * (n + 1) + b)]; };
            for (int a = 0; a < n; ++a)
                for (int b = 0; b < n; ++b)
                    AddCell(m, G(a, b), G(a, b + 1), G(a + 1, b + 1), G(a + 1, b), group);
        }
    }
    return m;
}

} // namespace scene
