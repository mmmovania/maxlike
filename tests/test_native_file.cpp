// maxlike's own scene files: every R3.1 sample, imported, saved and loaded back, is
// exactly the same scene (names, colours, links, matrices, every vertex and face, shared
// objects); modifier stacks keep their settings; saving again gives the same bytes.
#include "io/MaxImport.h"
#include "io/NativeFile.h"
#include "scene/Primitives.h"
#include "scene/DemoScene.h"
#include "scene/Modifier.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <set>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static bool SameMatrix(const math::Mat4& a, const math::Mat4& b) {
    for (const math::Vec3& p : {math::Vec3{0, 0, 0}, math::Vec3{1, 0, 0}, math::Vec3{0, 1, 0}, math::Vec3{0, 0, 1}})
        if (math::TransformPoint(a, p).x != math::TransformPoint(b, p).x || math::TransformPoint(a, p).y != math::TransformPoint(b, p).y ||
            math::TransformPoint(a, p).z != math::TransformPoint(b, p).z) return false;
    return true;
}

static bool SameMesh(const scene::Mesh& a, const scene::Mesh& b) {
    if (a.verts.size() != b.verts.size() || a.faces.size() != b.faces.size()) return false;
    for (size_t i = 0; i < a.verts.size(); ++i)
        if (a.verts[i].x != b.verts[i].x || a.verts[i].y != b.verts[i].y || a.verts[i].z != b.verts[i].z) return false;
    for (size_t i = 0; i < a.faces.size(); ++i) {
        const auto &f = a.faces[i], &g = b.faces[i];
        if (f.v[0] != g.v[0] || f.v[1] != g.v[1] || f.v[2] != g.v[2] || f.smoothing != g.smoothing ||
            f.materialId != g.materialId || f.edgeVisible != g.edgeVisible) return false;
    }
    return true;
}

// The loaded scene equals the original, node by node (in order).
static void Compare(const char* what, const scene::Scene& a, const scene::Scene& b) {
    CHECK(a.Nodes().size() == b.Nodes().size());
    if (a.Nodes().size() != b.Nodes().size()) return;
    std::set<const scene::Object*> objectsA, objectsB;
    for (size_t i = 0; i < a.Nodes().size(); ++i) {
        const scene::Node& n = *a.Nodes()[i];
        const scene::Node& m = *b.Nodes()[i];
        CHECK(n.Name() == m.Name() && n.WireColor() == m.WireColor());
        CHECK((n.Parent() ? n.Parent()->Name() : "") == (m.Parent() ? m.Parent()->Name() : ""));
        CHECK(SameMatrix(n.WorldMatrix(), m.WorldMatrix()) && SameMatrix(n.ObjectMatrix(), m.ObjectMatrix()));
        CHECK((n.GetObject() == nullptr) == (m.GetObject() == nullptr));
        if (n.GetObject() && m.GetObject()) {
            CHECK(SameMesh(n.GetObject()->Evaluate(), m.GetObject()->Evaluate()));
            CHECK(n.GetObject()->ClassName() == m.GetObject()->ClassName());
            objectsA.insert(n.GetObject().get());
            objectsB.insert(m.GetObject().get());
        }
    }
    CHECK(objectsA.size() == objectsB.size());                   // sharing kept
    if (failures) std::printf("  (in %s)\n", what);
}

int main() {
    for (const char* f : {"box", "box_editmesh", "boxes", "cone", "cylinder", "geosphere", "instances", "parent_child_link",
                          "pivot", "plane", "pyramid", "sphere", "teapot", "teapot_edit_mesh", "teapot_smoothinggroups",
                          "torus", "tube"}) {
        std::ifstream in(std::string(MAXLIKE_SAMPLES) + "/" + f + ".max", std::ios::binary);
        const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        scene::Scene original, loaded;
        io::ImportReport report;
        std::string error;
        CHECK(io::ImportMax(bytes, original, report, error));
        const std::vector<uint8_t> saved = io::SaveNative(original);
        CHECK(io::IsNativeFile(saved) && !io::IsNativeFile(bytes));
        CHECK(io::LoadNative(saved, loaded, error));
        Compare(f, original, loaded);
        CHECK(io::SaveNative(loaded) == saved);                    // saving again: the same bytes
    }
    // Modifier stacks: the deform demo (Bend, Taper, Twist), one switched off; an Edit
    // Mesh with a vertex selection.
    {
        scene::Scene original, loaded;
        CHECK(scene::BuildDemoScene(original, "deform"));
        auto twist = std::dynamic_pointer_cast<scene::DerivedObject>(original.Nodes()[1]->GetObject());
        CHECK(twist && !twist->Modifiers().empty());
        if (twist) twist->Modifiers()[0]->SetEnabled(false);
        std::string error;
        CHECK(io::LoadNative(io::SaveNative(original), loaded, error));
        Compare("deform", original, loaded);
        for (size_t i = 0; i < 3 && i < loaded.Nodes().size(); ++i) {
            auto a = std::dynamic_pointer_cast<scene::DerivedObject>(original.Nodes()[i]->GetObject());
            auto b = std::dynamic_pointer_cast<scene::DerivedObject>(loaded.Nodes()[i]->GetObject());
            CHECK(a && b && a->Modifiers().size() == b->Modifiers().size());
            if (!a || !b || a->Modifiers().empty() || b->Modifiers().empty()) continue;
            const auto &ma = *a->Modifiers()[0], &mb = *b->Modifiers()[0];
            CHECK(ma.ClassName() == mb.ClassName() && ma.Enabled() == mb.Enabled());
            for (size_t k = 0; k < ma.Params().Count(); ++k)
                if (ma.Params().Has(k)) CHECK(mb.Params().Has(k) && ma.Params().Float(k) == mb.Params().Float(k));
        }
    }
    {
        std::ifstream in(std::string(MAXLIKE_SAMPLES) + "/instances.max", std::ios::binary);
        const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        scene::Scene original, loaded;
        io::ImportReport report;
        std::string error;
        CHECK(io::ImportMax(bytes, original, report, error));
        auto d = std::dynamic_pointer_cast<scene::DerivedObject>(original.Nodes()[0]->GetObject());
        auto em = d ? std::dynamic_pointer_cast<scene::EditMeshModifier>(d->Modifiers()[0]) : nullptr;
        CHECK(em != nullptr);
        if (em) {
            em->SetSelectedVertices({4, 6});
            auto st = em->GetState();                       // Phase 9.4: faces selected and deleted
            st.selectedFaces = {0, 1};
            st.deletedFaces = {2, 3};
            st.deletedVertices = {7};
            st.hiddenFaces = {4};                           // Phase 9.6: hidden and welded
            st.hiddenVertices = {5};
            st.welds = {{6, 4}};
            em->SetState(st);
        }
        CHECK(io::LoadNative(io::SaveNative(original), loaded, error));
        auto d2 = std::dynamic_pointer_cast<scene::DerivedObject>(loaded.Nodes()[0]->GetObject());
        auto em2 = d2 ? std::dynamic_pointer_cast<scene::EditMeshModifier>(d2->Modifiers()[0]) : nullptr;
        CHECK(em2 && em2->SelectedVertices() == (std::vector<uint32_t>{4, 6}) && em2->Moves().size() == 4 && em2->ExpectedVertexCount() == 8);
        CHECK(em2 && em2->SelectedFaces() == (std::vector<uint32_t>{0, 1}) && em2->DeletedFaces() == (std::vector<uint32_t>{2, 3}) &&
              em2->DeletedVertices() == std::vector<uint32_t>{7});
        CHECK(em2 && em2->HiddenFaces() == std::vector<uint32_t>{4} && em2->HiddenVertices() == std::vector<uint32_t>{5} &&
              em2->Welds().size() == 1 && em2->Welds()[0] == std::make_pair(6u, 4u));
        CHECK(em2 && d2 && d2->Evaluate().verts.size() == 6);   // the deletion and the weld apply after loading
    }
    // Damaged or foreign input: an error, nothing created.
    {
        scene::Scene s;
        std::string error;
        CHECK(!io::LoadNative({1, 2, 3}, s, error) && s.Nodes().empty());
        std::vector<uint8_t> cut = io::SaveNative(s);
        cut.resize(10);
        CHECK(!io::LoadNative(cut, s, error));
    }
    // Group flags (Phase 10): a closed and an open group head survive saving and loading.
    {
        scene::Scene g, back;
        scene::Node* h1 = g.CreateNode("Group01");
        h1->SetGroup(true, false);
        scene::Node* h2 = g.CreateNode("Group02");
        h2->SetGroup(true, true);
        scene::Node* m = g.CreateNode("Box01");
        m->SetObject(std::make_shared<scene::BoxObject>(1, 1, 1));
        g.Link(m, h1);
        std::string error;
        CHECK(io::LoadNative(io::SaveNative(g), back, error));
        CHECK(back.Find("Group01") && back.Find("Group01")->IsGroupHead() && !back.Find("Group01")->GroupOpen());
        CHECK(back.Find("Group02") && back.Find("Group02")->IsGroupHead() && back.Find("Group02")->GroupOpen());
        CHECK(back.Find("Box01") && !back.Find("Box01")->IsGroupHead() && back.Find("Box01")->Parent() == back.Find("Group01"));
    }
    // A deformer's gizmo and centre (Phase 10.3) survive saving and loading.
    {
        scene::Scene g, back;
        auto bend = std::make_shared<scene::BendModifier>();
        bend->Params().Set(scene::BendModifier::kAngle, 45);
        scene::DeformModifier::Gizmo gz;
        gz.transform.position = {1, 2, 3};
        gz.transform.rotation = math::Quat::FromAxisAngle({0, 0, 1}, 0.25);
        gz.transform.scale = {1, 2, 0.5};
        gz.center = {0, 0, 7};
        bend->SetGizmo(gz);
        auto derived = std::make_shared<scene::DerivedObject>(std::make_shared<scene::BoxObject>(10, 10, 20));
        derived->AddModifier(bend);
        g.CreateNode("Box01")->SetObject(derived);
        std::string error;
        CHECK(io::LoadNative(io::SaveNative(g), back, error));
        auto d2 = back.Find("Box01") ? std::dynamic_pointer_cast<scene::DerivedObject>(back.Find("Box01")->GetObject()) : nullptr;
        auto b2 = d2 ? std::dynamic_pointer_cast<scene::BendModifier>(d2->Modifiers()[0]) : nullptr;
        CHECK(b2 && b2->GetGizmo().center.z == 7 && b2->GetGizmo().transform.scale.y == 2 && b2->GetGizmo().transform.position.x == 1);
        const scene::Mesh a = derived->Evaluate(), c = d2 ? d2->Evaluate() : scene::Mesh{};
        bool same = a.verts.size() == c.verts.size();
        for (size_t i = 0; same && i < a.verts.size(); ++i) same = math::Length(a.verts[i] - c.verts[i]) == 0;
        CHECK(same);
    }
    // Views and the selection (Phase 10.4): every field round-trips exactly.
    {
        scene::Scene g, back, back2;
        scene::Node* a = g.CreateNode("Box01");
        a->SetObject(std::make_shared<scene::BoxObject>(1, 1, 1));
        scene::Node* b = g.CreateNode("Box02");
        b->SetObject(std::make_shared<scene::BoxObject>(1, 1, 1));
        g.SetSelection({b->GetId()});
        io::NativeViews v;
        v.present = true;
        v.active = 3;
        const int types[4] = {6, 1, 5, 0};
        for (int i = 0; i < 4; ++i) {
            v.type[i] = types[i];
            v.ortho[i].width = 123.5 + i;
            v.ortho[i].centerU = -7.25 * i;
            v.ortho[i].centerV = 3.125 + i;
            v.perspective[i].target = {1.5 * i, -2.0, 0.5};
            v.perspective[i].distance = 321.0 + i;
            v.perspective[i].elevationDeg = 33.0 - i;
            v.perspective[i].azimuthDeg = -45.0 + 10 * i;
            v.perspective[i].fovDeg = 50.0;
        }
        std::string error;
        io::NativeViews r;
        CHECK(io::LoadNative(io::SaveNative(g, {a->GetId(), b->GetId()}, &v), back, error, &r));
        CHECK(r.present && r.active == 3);
        for (int i = 0; i < 4; ++i) {
            CHECK(r.type[i] == types[i] && r.ortho[i].width == v.ortho[i].width && r.ortho[i].centerU == v.ortho[i].centerU &&
                  r.ortho[i].centerV == v.ortho[i].centerV);
            CHECK(r.perspective[i].target.x == v.perspective[i].target.x && r.perspective[i].distance == v.perspective[i].distance &&
                  r.perspective[i].elevationDeg == v.perspective[i].elevationDeg && r.perspective[i].azimuthDeg == v.perspective[i].azimuthDeg &&
                  r.perspective[i].fovDeg == v.perspective[i].fovDeg);
        }
        CHECK(back.Selection().size() == 1 && back.FindById(back.Selection()[0])->Name() == "Box02");
        // Saved without views: none reported; the selection still comes back.
        io::NativeViews none;
        CHECK(io::LoadNative(io::SaveNative(g), back2, error, &none) && !none.present && back2.Selection().size() == 1);
    }
    if (failures == 0) std::printf("native file: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
