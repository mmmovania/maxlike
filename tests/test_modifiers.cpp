// Unit tests for the modifier stack and smoothing-group normals, modelled on the
// verified instances.max sample: seven nodes share one Derived Object (Box + Edit Mesh)
// whose Edit Mesh turns the top vertices 4-7 rigidly by +25 degrees.
#include "math/Quat.h"
#include "scene/Modifier.h"
#include "scene/Primitives.h"
#include "scene/Scene.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(const math::Vec3& a, const math::Vec3& b, double eps = 1e-9) {
    return std::fabs(a.x - b.x) < eps && std::fabs(a.y - b.y) < eps && std::fabs(a.z - b.z) < eps;
}
static constexpr double kPi = 3.14159265358979323846;

using namespace scene;

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

int main() {
    EditMeshMovesVertices();
    MismatchedCountIsSkipped();
    StackOrderAndDisable();
    SharedDerivedObject();
    SmoothingNormals();
    if (failures == 0) std::printf("modifiers: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
