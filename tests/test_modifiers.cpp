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
    // Another axis: Bend along X turns a row along X towards +Y.
    {
        scene::Mesh m;
        for (int i = 0; i <= 4; ++i) m.verts.push_back({25.0 * i, 0, 0});
        scene::BendModifier b;
        b.Params().Set(scene::BendModifier::kAngle, 90);
        b.Params().Set(scene::BendModifier::kAxis, 0);
        b.Apply(m);
        const double r = 100 / (kPi / 2);
        CHECK(Close(m.verts[4], {r, r, 0}));
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
    if (failures == 0) std::printf("modifiers: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
