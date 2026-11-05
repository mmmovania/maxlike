// Unit tests for the scene model: quaternions and transforms (with Max's
// conventions), hierarchy, object offsets, naming, instances, meshes, parameters.
#include "math/Quat.h"
#include "scene/Scene.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) <= eps; }
static bool Near(const math::Vec3& a, const math::Vec3& b, double eps = 1e-6) {
    return Near(a.x, b.x, eps) && Near(a.y, b.y, eps) && Near(a.z, b.z, eps);
}
static constexpr double kPi = 3.14159265358979323846;
static double Deg(double r) { return r * 180.0 / kPi; }

using namespace scene;

static void Quaternions() {
    const math::Quat q = math::Quat::FromAxisAngle({0, 0, 1}, kPi / 2);   // +90 deg about Z
    CHECK(Near(q.Rotate({1, 0, 0}), {0, 1, 0}));                           // X -> Y (right-handed)
    // Composition: (a * b) applies b first.
    const math::Quat x90 = math::Quat::FromAxisAngle({1, 0, 0}, kPi / 2);
    CHECK(Near((q * x90).Rotate({0, 1, 0}), q.Rotate(x90.Rotate({0, 1, 0}))));
    CHECK(Near(Deg(q.AngleAbout({0, 0, 1})), 90.0, 1e-9));
}

static void MaxFileRotation() {
    // instances.max, Box01: the file stores (0, 0, 0.3543, 0.9351); the Top viewport
    // shows the box rotated -41.5 degrees (max-format.md §5.2).
    const math::Quat q = math::Quat::FromMaxFile(0, 0, 0.3543, 0.9351);
    CHECK(Near(Deg(q.AngleAbout({0, 0, 1})), -41.5, 0.05));
}

static void TransformOrder() {
    // Scale, then rotate, then translate.
    PRS t;
    t.scale = {2, 2, 2};
    t.rotation = math::Quat::FromAxisAngle({0, 0, 1}, kPi / 2);
    t.position = {10, 0, 0};
    CHECK(Near(math::TransformPoint(t.Matrix(), {1, 0, 0}), {10, 2, 0}));
    // The inverse undoes it.
    const math::Mat4 m = t.Matrix();
    CHECK(Near(math::TransformPoint(math::AffineInverse(m), math::TransformPoint(m, {3, -4, 5})), {3, -4, 5}));
}

static void Hierarchy() {
    Scene s;
    Node* parent = s.CreateNode("Parent");
    Node* child = s.CreateNode("Child");
    parent->Transform().position = {100, 0, 0};
    parent->Transform().rotation = math::Quat::FromAxisAngle({0, 0, 1}, kPi / 2);
    child->Transform().position = {110, 0, 0};          // world position before linking
    CHECK(s.Link(child, parent));
    // Linking keeps the child where it was in the world...
    CHECK(Near(math::TransformPoint(child->WorldMatrix(), {0, 0, 0}), {110, 0, 0}));
    // ...and from now on it follows the parent.
    parent->Transform().position = {100, 50, 0};
    CHECK(Near(math::TransformPoint(child->WorldMatrix(), {0, 0, 0}), {110, 50, 0}));
    CHECK(child->Parent() == parent && parent->Children().size() == 1);
    // Cycles are refused.
    CHECK(!s.Link(parent, child));
    // Deleting the parent keeps the child's world placement and makes it top level.
    s.DeleteNode(parent);
    CHECK(child->Parent() == nullptr && s.Nodes().size() == 1);
    CHECK(Near(math::TransformPoint(child->WorldMatrix(), {0, 0, 0}), {110, 50, 0}, 1e-9));
}

static void MirroredLinkKeepsPlacement() {
    Scene s;
    Node* parent = s.CreateNode("P");
    Node* child = s.CreateNode("C");
    parent->Transform().scale = {-1, 1, 1};              // a mirrored parent
    child->Transform().position = {5, 6, 7};
    child->Transform().rotation = math::Quat::FromAxisAngle({1, 1, 0}, 0.7);
    const math::Vec3 before = math::TransformPoint(child->WorldMatrix(), {1, 2, 3});
    CHECK(s.Link(child, parent));
    CHECK(Near(math::TransformPoint(child->WorldMatrix(), {1, 2, 3}), before, 1e-9));
}

static void ObjectOffset() {
    Scene s;
    Node* n = s.CreateNode("Box01");
    n->Transform().position = {0, 0, 10};
    n->Transform().rotation = math::Quat::FromAxisAngle({0, 0, 1}, kPi / 2);
    n->ObjectOffset().position = {5, 0, 0};              // pivot moved 5 units off the object
    // The object's origin is 5 units along the node's local X, which is world +Y.
    CHECK(Near(math::TransformPoint(n->ObjectMatrix(), {0, 0, 0}), {0, 5, 10}));
}

static void Naming() {
    Scene s;
    CHECK(s.UniqueName("Box") == "Box01");
    s.CreateNode("Box01");
    s.CreateNode("Box02");
    CHECK(s.UniqueName("Box") == "Box03");
    s.DeleteNode(s.Find("Box01"));
    CHECK(s.UniqueName("Box") == "Box01");               // a freed number is reused
}

static Mesh Quad() {
    // One 10 x 10 quad in XY as two triangles, diagonal hidden (as Max draws quads).
    Mesh m;
    m.verts = {{0, 0, 0}, {10, 0, 0}, {10, 10, 0}, {0, 10, 0}};
    Face a; a.v[0] = 0; a.v[1] = 1; a.v[2] = 2; a.edgeVisible = 0x3;  // 0-1, 1-2 visible; 2-0 hidden
    Face b; b.v[0] = 2; b.v[1] = 3; b.v[2] = 0; b.edgeVisible = 0x3;  // 2-3, 3-0 visible; 0-2 hidden
    m.faces = {a, b};
    return m;
}

static void Instances() {
    Scene s;
    auto shared = std::make_shared<MeshObject>(Quad());
    Node* a = s.CreateNode("Box01");
    Node* b = s.CreateNode("Box02");
    a->SetObject(shared);
    b->SetObject(shared);
    CHECK(a->GetObject() == b->GetObject() && shared.use_count() == 3);
    shared->EditMesh().verts[0] = {-5, 0, 0};            // editing one instance edits both
    CHECK(Near(b->GetObject()->Evaluate().verts[0], {-5, 0, 0}));
}

static void Meshes() {
    const Mesh m = Quad();
    CHECK(m.Valid());
    const Bounds b = m.GetBounds();
    CHECK(Near(b.Size(), {10, 10, 0}));
    CHECK(Near(m.FaceNormal(0), {0, 0, 1}));             // counter-clockwise from +Z = facing +Z
    CHECK(m.VisibleEdges().size() == 4);                 // the hidden diagonal is not drawn
    Mesh bad = m;
    bad.faces[0].v[2] = 99;
    CHECK(!bad.Valid());
}

static void Parameters() {
    ParamBlock p;
    p.Define(0, ParamType::Float, 25.0);
    p.Define(3, ParamType::Int, 1);
    p.Define(6, ParamType::Bool, 0);
    CHECK(p.Float(0) == 25.0 && p.Int(3) == 1 && !p.Bool(6));
    p.Set(3, 4.6);                                       // ints round
    p.Set(6, 7);                                         // bools become 0/1
    p.Set(9, 1.0);                                       // undefined: ignored
    CHECK(p.Int(3) == 5 && p.Bool(6) && !p.Has(9) && p.Float(9, -1) == -1);
}

int main() {
    Quaternions();
    MaxFileRotation();
    TransformOrder();
    Hierarchy();
    MirroredLinkKeepsPlacement();
    ObjectOffset();
    Naming();
    Instances();
    Meshes();
    Parameters();
    if (failures == 0) std::printf("scene: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
