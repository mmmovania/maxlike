// Unit tests for the modifier stack and smoothing-group normals, modelled on the
// verified instances.max sample: seven nodes share one Derived Object (Box + Edit Mesh)
// whose Edit Mesh turns the top vertices 4-7 rigidly by +25 degrees.
#include "math/Quat.h"
#include "scene/Modifier.h"
#include "scene/Primitives.h"
#include "scene/Scene.h"
#include <cmath>
#include <cstdio>
#include <memory>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(const math::Vec3& a, const math::Vec3& b, double eps = 1e-9) {
    return std::fabs(a.x - b.x) < eps && std::fabs(a.y - b.y) < eps && std::fabs(a.z - b.z) < eps;
}

using namespace scene;
static constexpr double kPi = 3.14159265358979323846;

// An Edit Mesh that turns the top four vertices of a 1x1x1 box about Z.
static std::shared_ptr<EditMeshModifier> TwistTop(const Mesh& box, double degrees) {
    const math::Quat q = math::Quat::FromAxisAngle({0, 0, 1}, degrees * kPi / 180);
    std::vector<EditMeshModifier::Move> moves;
    for (uint32_t v = 4; v < 8; ++v) moves.push_back({v, q.Rotate(box.verts[v]) - box.verts[v]});
    return std::make_shared<EditMeshModifier>(box.verts.size(), moves);
}

static void EditMeshMovesVertices() {
    auto box = std::make_shared<BoxObject>(20, 20, 40);
    const Mesh base = box->Evaluate();
    auto derived = std::make_shared<DerivedObject>(box);
    derived->AddModifier(TwistTop(base, 25));
    const Mesh m = derived->Evaluate();
    // The bottom stays; the top is the same square turned by +25 degrees.
    for (uint32_t v = 0; v < 4; ++v) CHECK(Near(m.verts[v], base.verts[v]));
    const math::Quat q = math::Quat::FromAxisAngle({0, 0, 1}, 25 * kPi / 180);
    for (uint32_t v = 4; v < 8; ++v) CHECK(Near(m.verts[v], q.Rotate(base.verts[v])));
    CHECK(derived->Skipped().empty() && derived->ClassName() == "Box");
    // Changing the base object changes the result; the moves stay relative.
    box->Params().Set(BoxObject::kHeight, 50);
    CHECK(std::fabs(derived->Evaluate().GetBounds().max.z - 50) < 1e-9);
}

static void MismatchedCountIsSkipped() {
    auto box = std::make_shared<BoxObject>(20, 20, 40);
    auto derived = std::make_shared<DerivedObject>(box);
    derived->AddModifier(TwistTop(box->Evaluate(), 25));
    box->Params().Set(BoxObject::kHeightSegs, 3);            // now more vertices
    const Mesh m = derived->Evaluate();
    CHECK(m.verts.size() == box->Evaluate().verts.size());   // unchanged base mesh
    CHECK(derived->Skipped().size() == 1 && derived->Skipped()[0] == "Edit Mesh");
}

static void StackOrderAndDisable() {
    auto box = std::make_shared<BoxObject>(10, 10, 10);
    const Mesh base = box->Evaluate();
    auto derived = std::make_shared<DerivedObject>(box);
    derived->AddModifier(std::make_shared<EditMeshModifier>(8, std::vector<EditMeshModifier::Move>{{0, {1, 0, 0}}}));
    auto second = std::make_shared<EditMeshModifier>(8, std::vector<EditMeshModifier::Move>{{0, {0, 2, 0}}});
    derived->AddModifier(second);
    CHECK(Near(derived->Evaluate().verts[0], base.verts[0] + math::Vec3{1, 2, 0}));
    second->SetEnabled(false);
    CHECK(Near(derived->Evaluate().verts[0], base.verts[0] + math::Vec3{1, 0, 0}));
}

static void SharedDerivedObject() {
    // Seven nodes, one Derived Object; each node has its own rotation (Box01 -41.5,
    // Box07 +47.0 degrees in the sample).
    Scene s;
    auto box = std::make_shared<BoxObject>(20, 20, 40);
    auto derived = std::make_shared<DerivedObject>(box);
    derived->AddModifier(TwistTop(box->Evaluate(), 25));
    for (int i = 0; i < 7; ++i) {
        Node* n = s.CreateNode(s.UniqueName("Box"));
        n->SetObject(derived);
        const double deg = -41.5 + (47.0 + 41.5) * i / 6;
        n->Transform().rotation = math::Quat::FromAxisAngle({0, 0, 1}, deg * kPi / 180);
    }
    CHECK(s.Find("Box01")->GetObject() == s.Find("Box07")->GetObject() && derived.use_count() == 8);
    CHECK(std::fabs(math::Quat(s.Find("Box07")->Transform().rotation).AngleAbout({0, 0, 1}) * 180 / kPi - 47.0) < 1e-9);
}

static void SmoothingNormals() {
    // Box: each side its own group -> every corner normal is its face normal (flat).
    const Mesh box = BoxObject(10, 10, 10).Evaluate();
    const auto bn = box.CornerNormals();
    for (size_t i = 0; i < box.faces.size(); ++i)
        for (int k = 0; k < 3; ++k) CHECK(Near(bn[i * 3 + k], box.FaceNormal(i)));
    // Sphere, one group: corner normals point (nearly) straight out from the centre.
    const Mesh sphere = SphereObject(10).Evaluate();
    const auto sn = sphere.CornerNormals();
    double worst = 1;
    for (size_t i = 0; i < sphere.faces.size(); ++i)
        for (int k = 0; k < 3; ++k)
            worst = std::min(worst, math::Dot(sn[i * 3 + k], math::Normalize(sphere.verts[sphere.faces[i].v[k]])));
    CHECK(worst > 0.99);
    // Twisted box (instances.max): each side is a non-planar quad split in two; both
    // triangles share the side's group, so on the diagonal's vertices they share one
    // averaged normal (smooth across the diagonal), unlike their own face normals.
    auto b = std::make_shared<BoxObject>(20, 20, 40);
    DerivedObject d(b);
    d.AddModifier(TwistTop(b->Evaluate(), 25));
    const Mesh t = d.Evaluate();
    const auto tn = t.CornerNormals();
    // Front side: faces 4 (0 1 5) and 5 (5 4 0); shared diagonal vertices 0 and 5.
    CHECK(Near(tn[4 * 3 + 0], tn[5 * 3 + 2]));              // vertex 0 in both faces
    CHECK(Near(tn[4 * 3 + 2], tn[5 * 3 + 0]));              // vertex 5 in both faces
    CHECK(!Near(t.FaceNormal(4), t.FaceNormal(5), 1e-3));    // the two halves are not coplanar
    // Flat faces (group 0) keep their own normal.
    Mesh flat = t;
    for (Face& f : flat.faces) f.smoothing = 0;
    const auto fnorm = flat.CornerNormals();
    CHECK(Near(fnorm[4 * 3 + 0], flat.FaceNormal(4)));
}

// ---------------------------------------------------------------- Bend / Taper / Twist
static bool Close(const math::Vec3& a, const math::Vec3& b, double eps = 1e-9) {
    return std::fabs(a.x - b.x) < eps && std::fabs(a.y - b.y) < eps && std::fabs(a.z - b.z) < eps;
}
// A column of points along Z from 0 to 100 (an object standing on its pivot).
static scene::Mesh Column() {
    scene::Mesh m;
    for (int i = 0; i <= 10; ++i) m.verts.push_back({0, 0, 10.0 * i});
    return m;
}

static void Deformers() {
    // Zero settings leave the mesh unchanged.
    {
        scene::Mesh m = Column(), ref = Column();
        scene::BendModifier b; scene::TwistModifier t; scene::TaperModifier p;
        CHECK(b.Apply(m) && t.Apply(m) && p.Apply(m));
        bool same = true;
        for (size_t i = 0; i < m.verts.size(); ++i) same = same && Close(m.verts[i], ref.verts[i]);
        CHECK(same);
    }
    // Bend 90 degrees: the base stays; the top turns to point along +X at (r, 0, r),
    // r = L / angle; every point stays on the arc and the arc keeps its length.
    {
        scene::Mesh m = Column();
        scene::BendModifier b;
        b.Params().Set(scene::BendModifier::kAngle, 90);
        CHECK(b.Apply(m));
        const double r = 100 / (kPi / 2);
        CHECK(Close(m.verts[0], {0, 0, 0}) && Close(m.verts[10], {r, 0, r}));
        double length = 0;
        for (size_t i = 1; i < m.verts.size(); ++i) {
            CHECK(std::fabs(math::Length(m.verts[i] - math::Vec3{r, 0, 0}) - r) < 1e-9);   // on the arc
            const double chord = math::Length(m.verts[i] - m.verts[i - 1]), phi = 10 / r;
            length += r * 2 * std::asin(chord / (2 * r)) * (phi > 0 ? 1 : 1);
        }
        CHECK(std::fabs(length - 100) < 1e-6);                                         // arc length kept
        // Direction 90: the same bend towards +Y.
        scene::Mesh d = Column();
        b.Params().Set(scene::BendModifier::kDirection, 90);
        b.Apply(d);
        CHECK(Close(d.verts[10], {0, r, r}));
    }
    // Bend with limits: above the upper limit the column continues straight on.
    {
        scene::Mesh m = Column();
        scene::BendModifier b;
        b.Params().Set(scene::BendModifier::kAngle, 90);
        b.Params().Set(scene::BendModifier::kLimit, 1);
        b.Params().Set(scene::BendModifier::kUpper, 50);
        b.Apply(m);
        const math::Vec3 d1 = m.verts[8] - m.verts[7], d2 = m.verts[10] - m.verts[9];
        CHECK(Close(d1, d2) && std::fabs(math::Length(d1) - 10) < 1e-9);               // straight, unstretched
        CHECK(Close(m.verts[3], [] { scene::Mesh c = Column(); scene::BendModifier u;
                                     u.Params().Set(scene::BendModifier::kAngle, 90); u.Apply(c); return c.verts[3]; }()));
    }
    // Twist 90 degrees: the top's corner (10, 0, 100) turns to (0, 10, 100); the base stays.
    {
        scene::Mesh m;
        m.verts = {{10, 0, 0}, {10, 0, 100}, {10, 0, 50}};
        scene::TwistModifier t;
        t.Params().Set(scene::TwistModifier::kAngle, 90);
        t.Apply(m);
        CHECK(Close(m.verts[0], {10, 0, 0}) && Close(m.verts[1], {0, 10, 100}));
        CHECK(Close(m.verts[2], {10 * std::cos(kPi / 4), 10 * std::sin(kPi / 4), 50}));
    }
    // Taper amount 1: the top doubles, the middle grows by half; Curve bows the middle only.
    {
        scene::Mesh m;
        m.verts = {{10, 10, 0}, {10, 10, 100}, {10, 10, 50}};
        scene::TaperModifier p;
        p.Params().Set(scene::TaperModifier::kAmount, 1);
        p.Apply(m);
        CHECK(Close(m.verts[0], {10, 10, 0}) && Close(m.verts[1], {20, 20, 100}) && Close(m.verts[2], {15, 15, 50}));
        scene::Mesh c;
        c.verts = {{10, 10, 0}, {10, 10, 100}, {10, 10, 50}};
        scene::TaperModifier q;
        q.Params().Set(scene::TaperModifier::kCurve, 1);
        q.Apply(c);
        CHECK(Close(c.verts[0], {10, 10, 0}) && Close(c.verts[1], {10, 10, 100}) && Close(c.verts[2], {20, 20, 50}));
    }
    // Another axis: Bend along X turns a row along X towards -Z (11.4: measured against
    // R3.1's bend.max; Phase 7 had +Y, from maxlike's own formula, never checked in R3).
    {
        scene::Mesh m;
        for (int i = 0; i <= 4; ++i) m.verts.push_back({25.0 * i, 0, 0});
        scene::BendModifier b;
        b.Params().Set(scene::BendModifier::kAngle, 90);
        b.Params().Set(scene::BendModifier::kAxis, 0);
        b.Apply(m);
        const double r = 100 / (kPi / 2);
        CHECK(Close(m.verts[4], {r, 0, -r}));
    }
    // Against R3.1 (screenshot r3.1-modify-bend.png): Box 62.572 wide, 60.718 tall, Bend
    // 30.5 degrees about Z. R3 draws the top corners of the bent box's straight edges at
    // pixels that convert (Front view: 5.9132 px per unit, pivot at x 1303, base row 495)
    // to the right corner (42.79, 41.94) and the left edge passing (-29.25, 7.61),
    // (-24.52, 24.52) and (-15.39, 58.35) in (x, z); ours must agree within a pixel.
    {
        scene::Mesh m;
        m.verts = {{31.286, 0, 60.718}, {-31.286, 0, 60.718}, {31.286, 0, 0}, {-31.286, 0, 0}};   // base too: the extent
        scene::BendModifier b;
        b.Params().Set(scene::BendModifier::kAngle, 30.5);
        b.Apply(m);
        const double px = 1 / 5.9132;                                       // one pixel in units
        CHECK(std::fabs(m.verts[0].x - 42.79) < px && std::fabs(m.verts[0].z - 41.94) < px);
        // The left edge runs from (-31.286, 0) to the bent left top corner; check it passes
        // R3's measured points.
        const double x0 = -31.286, x1 = m.verts[1].x, z1 = m.verts[1].z;
        const double pts[3][2] = {{-29.25, 7.61}, {-24.52, 24.52}, {-15.39, 58.35}};
        for (const auto& pt : pts) {
            const double xOnLine = x0 + (x1 - x0) * (pt[1] / z1);
            CHECK(std::fabs(xOnLine - pt[0]) < px);
        }
    }
    // On a real object through the stack: a box keeps its vertex count; bending changes it.
    {
        auto d = std::make_shared<scene::DerivedObject>(std::make_shared<scene::BoxObject>(20, 20, 100));
        auto bend = std::make_shared<scene::BendModifier>();
        bend->Params().Set(scene::BendModifier::kAngle, 60);
        d->AddModifier(bend);
        const scene::Mesh m = d->Evaluate();
        CHECK(m.verts.size() == 8 && m.faces.size() == 12 && d->Skipped().empty());
    }
}

int main() {
    Deformers();
    EditMeshMovesVertices();
    MismatchedCountIsSkipped();
    StackOrderAndDisable();
    SharedDerivedObject();
    SmoothingNormals();
    // The gizmo and centre (10.3).
    {
        auto box = std::make_shared<scene::BoxObject>(20, 20, 40);
        const scene::Mesh input = box->Evaluate();
        auto near = [](const math::Vec3& a, const math::Vec3& b) { return math::Length(a - b) < 1e-9; };
        scene::BendModifier bend;
        bend.Params().Set(scene::BendModifier::kAngle, 90);
        // A centre offset is the same as moving the gizmo by it.
        scene::DeformModifier::Gizmo byCenter, byGizmo;
        byCenter.center = {0, 0, 10};
        byGizmo.transform.position = {0, 0, 10};
        scene::Mesh m1 = input, m2 = input;
        bend.SetGizmo(byCenter); bend.Apply(m1);
        bend.SetGizmo(byGizmo);  bend.Apply(m2);
        bool same = true;
        for (size_t i = 0; i < m1.verts.size(); ++i) same = same && near(m1.verts[i], m2.verts[i]);
        CHECK(same);
        // A translated gizmo is: shift into it, deform, shift back.
        scene::Mesh shifted = input, manual;
        for (auto& v : shifted.verts) v = v - math::Vec3{0, 0, 10};
        bend.SetGizmo({});
        bend.Apply(shifted);
        for (auto& v : shifted.verts) v = v + math::Vec3{0, 0, 10};
        same = true;
        for (size_t i = 0; i < m2.verts.size(); ++i) same = same && near(m2.verts[i], shifted.verts[i]);
        CHECK(same);
        // The lattice: with no angle exactly the box's twelve edges (8 pieces each)...
        scene::BendModifier flat;
        const auto lines = flat.GizmoLines(input, 8);
        CHECK(lines.size() == 96);
        const scene::Bounds b = input.GetBounds();
        for (const auto& [p, q] : lines)
            for (const math::Vec3& v : {p, q})
                CHECK(v.x >= b.min.x - 1e-9 && v.x <= b.max.x + 1e-9 && v.z >= b.min.z - 1e-9 && v.z <= b.max.z + 1e-9);
        // ...and bent, its corners are exactly the bent box's corners.
        scene::Mesh bent = input;
        bend.Apply(bent);
        const auto bentLines = bend.GizmoLines(input, 8);
        for (const auto& corner : bent.verts) {
            bool found = false;
            for (const auto& [p, q] : bentLines) found = found || near(p, corner) || near(q, corner);
            CHECK(found);
        }
        // The centre in object space follows the gizmo.
        scene::DeformModifier::Gizmo g;
        g.transform.position = {5, 0, 0};
        g.transform.rotation = math::Quat::FromAxisAngle({0, 0, 1}, 3.14159265358979323846 / 2);
        g.center = {1, 0, 0};
        bend.SetGizmo(g);
        CHECK(near(bend.CenterPoint(), {5, 1, 0}));
    }
    if (failures == 0) std::printf("modifiers: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
