#include "scene/Primitives.h"
#include <algorithm>
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

} // namespace scene
