// io::ImportMax against the R3.1 samples: each imported node's transforms agree with the
// file's (io::ReadNodes and io::ObjectMatrix, an independent route), links, wire colours
// and names are kept, instances share one object, and nothing is left unimported.
#include "io/MaxImport.h"
#include "io/MaxNodes.h"
#include <cmath>
#include <cstdio>
#include "scene/Groups.h"
#include "scene/Primitives.h"
#include "scene/Modifier.h"
#include <fstream>
#include <iterator>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static std::vector<uint8_t> Load(const std::string& name) {
    std::ifstream f(std::string(MAXLIKE_SAMPLES) + "/" + name + ".max", std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static bool SameMatrix(const math::Mat4& a, const math::Mat4& b) {
    for (const math::Vec3& p : {math::Vec3{0, 0, 0}, math::Vec3{10, 0, 0}, math::Vec3{0, 10, 0}, math::Vec3{0, 0, 10}})
        if (math::Length(math::TransformPoint(a, p) - math::TransformPoint(b, p)) > 1e-4) return false;
    return true;
}

int main() {
    struct Expect { const char* file; size_t nodes, objects; };
    const Expect all[] = {{"box", 1, 1}, {"box_editmesh", 1, 1}, {"boxes", 4, 4}, {"cone", 1, 1}, {"cylinder", 1, 1},
                          {"geosphere", 1, 1}, {"instances", 7, 1}, {"parent_child_link", 2, 2}, {"pivot", 1, 1},
                          {"plane", 1, 1}, {"pyramid", 1, 1}, {"sphere", 1, 1}, {"teapot", 1, 1}, {"teapot_edit_mesh", 1, 1},
                          {"teapot_smoothinggroups", 1, 1}, {"torus", 1, 1}, {"tube", 1, 1}};
    for (const Expect& e : all) {
        const auto bytes = Load(e.file);
        scene::Scene scene;
        io::ImportReport report;
        std::string error;
        CHECK(io::ImportMax(bytes, scene, report, error));
        CHECK(report.nodes == e.nodes && report.objects == e.objects && scene.Nodes().size() == e.nodes);
        CHECK(report.notes.empty());
        for (const auto& n : report.notes) std::printf("  %s: %s\n", e.file, n.c_str());
        // Every node: the same name, colour and transforms as the file says.
        io::MaxScene file;
        CHECK(file.Open(bytes, error));
        const auto nodes = io::ReadNodes(file);
        for (size_t i = 0; i < nodes.size(); ++i) {
            const scene::Node* n = scene.Find(nodes[i].name);
            CHECK(n != nullptr);
            if (!n) continue;
            CHECK(n->GetObject() != nullptr);
            CHECK(n->WireColor() == ((uint32_t{nodes[i].wire[0]} << 16) | (uint32_t{nodes[i].wire[1]} << 8) | nodes[i].wire[2]));
            CHECK(SameMatrix(n->WorldMatrix(), io::WorldMatrix(nodes, i)));
            CHECK(SameMatrix(n->ObjectMatrix(), io::ObjectMatrix(nodes, i)));
            CHECK((n->Parent() == nullptr) == (nodes[i].parent < 0));
        }
    }
    // The hierarchy: Box01 under Sphere01; instances: seven nodes on one object.
    {
        scene::Scene scene;
        io::ImportReport report;
        std::string error;
        CHECK(io::ImportMax(Load("parent_child_link"), scene, report, error));
        const scene::Node* box = scene.Find("Box01");
        CHECK(box && box->Parent() && box->Parent()->Name() == "Sphere01");
        CHECK(box && box->Parent() && box->Parent()->Children().size() == 1);
    }
    {
        scene::Scene scene;
        io::ImportReport report;
        std::string error;
        CHECK(io::ImportMax(Load("instances"), scene, report, error));
        const scene::Object* shared = scene.Nodes().empty() ? nullptr : scene.Nodes()[0]->GetObject().get();
        for (const auto& n : scene.Nodes()) CHECK(n->GetObject().get() == shared && shared != nullptr);
        CHECK(scene.Find("Box07") && scene.Find("Box07")->WireColor() == 0xB11A58);   // 177, 26, 88
    }
    // Not a .max file: an error, and the scene untouched.
    {
        scene::Scene scene;
        io::ImportReport report;
        std::string error;
        CHECK(!io::ImportMax({0x50, 0x4B, 3, 4}, scene, report, error) && !error.empty() && scene.Nodes().empty());
    }
    // 11.4: Bend, Taper and Twist from R3.1 files, each stack compared with the same stack
    // made in maxlike (a 20 x 20 x 60 box, 10 height segments). The samples were built one on
    // another: taper.max is Bend (at its defaults) under Taper, twist.max Bend and Taper under
    // Twist; only the top modifier carries the values entered.
    {
        using Params = std::vector<std::pair<int, double>>;
        struct Expect { std::shared_ptr<scene::DeformModifier> mod; Params params; };
        auto check = [&](const char* file, std::vector<Expect> stack) {
            scene::Scene scene;
            io::ImportReport report;
            std::string error;
            CHECK(io::ImportMax(Load(file), scene, report, error));
            const scene::Node* box = scene.Find("Box01");
            auto derived = box ? std::dynamic_pointer_cast<scene::DerivedObject>(box->GetObject()) : nullptr;
            CHECK(derived && derived->Modifiers().size() == stack.size());
            if (!derived || derived->Modifiers().size() != stack.size()) return std::shared_ptr<scene::DeformModifier>();
            auto base = std::make_shared<scene::BoxObject>(20, 20, 60);
            base->Params().Set(scene::BoxObject::kHeightSegs, 10);
            scene::DerivedObject made(base);
            std::shared_ptr<scene::DeformModifier> top;
            for (size_t k = 0; k < stack.size(); ++k) {
                auto mod = std::dynamic_pointer_cast<scene::DeformModifier>(derived->Modifiers()[k]);
                CHECK(mod && mod->ClassName() == stack[k].mod->ClassName());
                if (!mod) return top;
                for (const auto& [i, v] : stack[k].params) {
                    CHECK(std::fabs(mod->Params().Float(static_cast<size_t>(i)) - v) < 1e-5);
                    stack[k].mod->Params().Set(static_cast<size_t>(i), v);
                }
                stack[k].mod->SetGizmo(mod->GetGizmo());
                made.AddModifier(stack[k].mod);
                top = mod;
            }
            const scene::Mesh a = box->GetObject()->Evaluate(), b = made.Evaluate();
            CHECK(!a.verts.empty() && a.verts.size() == b.verts.size());
            double worst = 0;
            for (size_t i = 0; i < a.verts.size() && i < b.verts.size(); ++i) worst = std::max(worst, math::Length(a.verts[i] - b.verts[i]));
            if (worst >= 1e-6) std::printf("  %s: largest difference %g over %zu vertices\n", file, worst, a.verts.size());
            CHECK(worst < 1e-6);
            return top;
        };
        using B = scene::BendModifier; using T = scene::TaperModifier; using W = scene::TwistModifier;
        check("bend", {{std::make_shared<B>(), {{B::kAngle, 45}, {B::kDirection, 30}, {B::kAxis, 0}}}});
        check("taper", {{std::make_shared<B>(), {{B::kAngle, 0}}},
                        {std::make_shared<T>(), {{T::kAmount, 0.5}, {T::kCurve, 0.3}, {T::kAxis, 2}, {T::kEffect, 2}}}});
        check("twist", {{std::make_shared<B>(), {{B::kAngle, 0}}}, {std::make_shared<T>(), {{T::kAmount, 0}}},
                        {std::make_shared<W>(), {{W::kAngle, 90}, {W::kBias, 20}, {W::kAxis, 2}}}});
        // bend-gizmo.max: the gizmo dragged to about (9.77, 0.23, 0.5) and turned 30 degrees
        // about Y (as stored); the centre at its default.
        auto g = check("bend-gizmo", {{std::make_shared<B>(), {{B::kAngle, 45}, {B::kDirection, 30}, {B::kAxis, 0}}}});
        if (g) {
            const auto& gz = g->GetGizmo();
            CHECK(std::fabs(gz.transform.position.x - 9.76825) < 1e-4 && std::fabs(gz.transform.position.y - 0.23175) < 1e-4 &&
                  std::fabs(gz.transform.position.z - 0.5) < 1e-4);
            const math::Vec3 x = gz.transform.rotation.Rotate({1, 0, 0}), y = gz.transform.rotation.Rotate({0, 1, 0});
            CHECK(std::fabs(y.y - 1) < 1e-6);                                                      // about Y
            CHECK(std::fabs(std::fabs(x.z) - 0.5) < 1e-5 && std::fabs(x.x - std::sqrt(3.0) / 2) < 1e-5);   // by 30 degrees
            CHECK(math::Length(gz.center) < 1e-9);
        }
    }
    // 11.4b: groups, Backface Cull and a material's colour from R3.1 files.
    {
        scene::Scene g;
        io::ImportReport report;
        std::string error;
        CHECK(io::ImportMax(Load("group"), g, report, error) && g.Nodes().size() == 3);
        const scene::Node* head = g.Find("Group01");
        CHECK(head && head->IsGroupHead() && !head->GroupOpen() && !head->GetObject() && head->Children().size() == 2);
        if (head && !head->Children().empty())
            CHECK(scene::groups::SelectableFor(const_cast<scene::Node*>(head->Children()[0])) == head);   // a click selects the group
        // Backface Cull: off only in backface-off.max; on in every node of the others (the
        // bit is 0x00800000, not 0x800, which clones lack).
        scene::Scene bf, box, inst;
        CHECK(io::ImportMax(Load("backface-off"), bf, report, error) && bf.Nodes().size() == 1 && !bf.Nodes()[0]->BackfaceCull());
        CHECK(io::ImportMax(Load("box"), box, report, error) && box.Nodes()[0]->BackfaceCull());
        CHECK(io::ImportMax(Load("instances"), inst, report, error));
        for (const auto& n : inst.Nodes()) CHECK(n->BackfaceCull());
        // material.max: a Standard material with a red diffuse (0.992, 0, 0) -> 253, 0, 0.
        scene::Scene mat;
        CHECK(io::ImportMax(Load("material"), mat, report, error));
        const scene::Node* m = mat.Find("Box01");
        CHECK(m && m->HasMaterialColor() && m->MaterialColor() == 0xFD0000);
        CHECK(!box.Nodes()[0]->HasMaterialColor());
    }
    if (failures == 0) std::printf("max import: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
