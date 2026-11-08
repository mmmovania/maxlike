// io::ImportMax against the R3.1 samples: each imported node's transforms agree with the
// file's (io::ReadNodes and io::ObjectMatrix, an independent route), links, wire colours
// and names are kept, instances share one object, and nothing is left unimported.
#include "io/MaxImport.h"
#include "io/MaxNodes.h"
#include <cmath>
#include <cstdio>
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
    if (failures == 0) std::printf("max import: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
