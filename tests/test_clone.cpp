// Edit > Clone, File > Summary Info and File > Save Selected (Phase 9.3), on the samples.
#include "io/MaxImport.h"
#include "io/NativeFile.h"
#include "scene/Clone.h"
#include "scene/Modifier.h"
#include "scene/Primitives.h"
#include "scene/Summary.h"
#include "scene/Undo.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <set>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static bool Import(const char* name, scene::Scene& s) {
    std::ifstream f(std::string(MAXLIKE_SAMPLES) + "/" + name + ".max", std::ios::binary);
    const std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    io::ImportReport r;
    std::string e;
    return io::ImportMax(b, s, r, e);
}
static bool SameMatrix(const math::Mat4& a, const math::Mat4& b, double eps = 1e-9) {
    for (const math::Vec3& p : {math::Vec3{0, 0, 0}, math::Vec3{10, 0, 0}, math::Vec3{0, 10, 0}, math::Vec3{0, 0, 10}})
        if (math::Length(math::TransformPoint(a, p) - math::TransformPoint(b, p)) > eps) return false;
    return true;
}
static bool SameMesh(const scene::Mesh& a, const scene::Mesh& b) {
    if (a.verts.size() != b.verts.size() || a.faces.size() != b.faces.size()) return false;
    for (size_t i = 0; i < a.verts.size(); ++i) if (math::Length(a.verts[i] - b.verts[i]) != 0) return false;
    return true;
}

int main() {
    // CopyObject: an Edit Mesh over a Box, copied, evaluates the same and is independent.
    {
        scene::Scene s;
        CHECK(Import("instances", s));
        const auto original = s.Nodes()[0]->GetObject();
        const auto copy = scene::CopyObject(original);
        CHECK(copy && copy != original && SameMesh(copy->Evaluate(), original->Evaluate()));
        auto d = std::dynamic_pointer_cast<scene::DerivedObject>(copy);
        auto d0 = std::dynamic_pointer_cast<scene::DerivedObject>(original);
        CHECK(d && d0 && d->Base() != d0->Base() && d->Modifiers()[0] != d0->Modifiers()[0]);
        if (d) d->Base()->Params().Set(scene::BoxObject::kHeight, 99);
        CHECK(d0 && d0->Base()->Params().Float(scene::BoxObject::kHeight) != 99);
        // Edit Mesh's whole state comes along (Phase 9.4: deleted faces).
        auto em0 = std::dynamic_pointer_cast<scene::EditMeshModifier>(d0->Modifiers()[0]);
        auto st = em0->GetState();
        st.deletedFaces = {0, 1};
        em0->SetState(st);
        const auto copy2 = scene::CopyObject(original);
        CHECK(copy2->Evaluate().FaceCount() == original->Evaluate().FaceCount() && original->Evaluate().FaceCount() == 10);
    }
    // Clone a parent and its child: Sphere02 and Box02, Box02 under Sphere02, exactly on the
    // originals; the child alone keeps the original parent.
    {
        scene::Scene s;
        CHECK(Import("parent_child_link", s));
        const scene::Node* sphere = s.Find("Sphere01");
        const scene::Node* box = s.Find("Box01");
        const auto ids = scene::CloneNodes(s, {box->GetId(), sphere->GetId()}, false);
        CHECK(ids.size() == 2 && s.Nodes().size() == 4);
        const scene::Node* sphere2 = s.Find("Sphere02");
        const scene::Node* box2 = s.Find("Box02");
        CHECK(sphere2 && box2 && box2->Parent() == sphere2 && ids[0] == sphere2->GetId());
        CHECK(box2 && SameMatrix(box2->ObjectMatrix(), box->ObjectMatrix()) && SameMatrix(sphere2->ObjectMatrix(), sphere->ObjectMatrix()));
        CHECK(box2 && box2->GetObject() != box->GetObject() && box2->WireColor() == box->WireColor());
        const auto alone = scene::CloneNodes(s, {box->GetId()}, true, "Lonely");
        const scene::Node* lonely = s.Find("Lonely");
        CHECK(alone.size() == 1 && lonely && lonely->Parent() == sphere && lonely->GetObject() == box->GetObject());
        CHECK(lonely && SameMatrix(lonely->ObjectMatrix(), box->ObjectMatrix()));
        // Undo / redo through the Merge machinery.
        scene::UndoStack undo;
        undo.Execute(std::make_unique<scene::AddedNodesCommand>("Clone", ids), s);
        CHECK(undo.Undo(s) && s.Nodes().size() == 3 && !s.Find("Box02"));
        CHECK(undo.Redo(s) && s.Nodes().size() == 5 && s.Find("Box02") && s.Find("Box02")->Parent() == s.Find("Sphere02"));
    }
    // Copying seven instances: seven new boxes sharing one new object.
    {
        scene::Scene s;
        CHECK(Import("instances", s));
        std::vector<scene::Node::Id> all;
        for (const auto& n : s.Nodes()) all.push_back(n->GetId());
        const auto ids = scene::CloneNodes(s, all, false);
        std::set<const scene::Object*> objects;
        for (const auto& n : s.Nodes()) objects.insert(n->GetObject().get());
        CHECK(ids.size() == 7 && objects.size() == 2 && s.Find("Box14"));
    }
    // Summary Info: R3.1's own dialog shows Teapot01 (Teapot) 530 / 1024 for teapot.max.
    {
        struct E { const char* file; const char* name; const char* type; size_t verts, faces; };
        const E expected[] = {{"teapot", "Teapot01", "Teapot", 530, 1024}, {"cone", "Cone01", "Cone", 146, 288},
                              {"cylinder", "Cylinder01", "Cylinder", 110, 216}, {"sphere", "Sphere01", "Sphere", 482, 960},
                              {"geosphere", "GeoSphere01", "GeoSphere", 162, 320}, {"torus", "Torus01", "Torus", 288, 576},
                              {"tube", "Tube01", "Tube", 216, 432}};
        for (const E& e : expected) {
            scene::Scene s;
            CHECK(Import(e.file, s));
            const scene::Summary sum = scene::Summarize(s);
            CHECK(sum.objects == 1 && sum.total == 1 && sum.verts == e.verts && sum.faces == e.faces);
            CHECK(sum.rows.size() == 1 && sum.rows[0].name == e.name && sum.rows[0].type == e.type);
        }
        scene::Scene s;
        CHECK(Import("boxes", s));
        const scene::Summary sum = scene::Summarize(s);
        CHECK(sum.objects == 4 && sum.verts == 32 && sum.faces == 48);
    }
    // Save Selected: the child alone becomes top-level and stays where it was.
    {
        scene::Scene s, loaded, both;
        CHECK(Import("parent_child_link", s));
        const scene::Node* box = s.Find("Box01");
        std::string error;
        CHECK(io::LoadNative(io::SaveNative(s, {box->GetId()}), loaded, error));
        CHECK(loaded.Nodes().size() == 1 && loaded.Find("Box01") && !loaded.Find("Box01")->Parent());
        CHECK(loaded.Find("Box01") && SameMatrix(loaded.Find("Box01")->ObjectMatrix(), box->ObjectMatrix(), 1e-6));
        CHECK(io::LoadNative(io::SaveNative(s, {box->GetId(), s.Find("Sphere01")->GetId()}), both, error));
        CHECK(both.Nodes().size() == 2 && both.Find("Box01")->Parent() == both.Find("Sphere01"));
    }
    if (failures == 0) std::printf("clone: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
