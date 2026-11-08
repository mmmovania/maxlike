// maxlike's own scene files: every R3.1 sample, imported, saved and loaded back, is
// exactly the same scene (names, colours, links, matrices, every vertex and face, shared
// objects); modifier stacks keep their settings; saving again gives the same bytes.
#include "io/MaxImport.h"
#include "io/NativeFile.h"
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
        if (em) em->SetSelectedVertices({4, 6});
        CHECK(io::LoadNative(io::SaveNative(original), loaded, error));
        auto d2 = std::dynamic_pointer_cast<scene::DerivedObject>(loaded.Nodes()[0]->GetObject());
        auto em2 = d2 ? std::dynamic_pointer_cast<scene::EditMeshModifier>(d2->Modifiers()[0]) : nullptr;
        CHECK(em2 && em2->SelectedVertices() == (std::vector<uint32_t>{4, 6}) && em2->Moves().size() == 4 && em2->ExpectedVertexCount() == 8);
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
    if (failures == 0) std::printf("native file: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
