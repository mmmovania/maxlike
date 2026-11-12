// File > Merge: io::MergeScene and its undo step (scene::AddedNodesCommand), on the samples.
#include "io/Merge.h"
#include "scene/Undo.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <set>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static std::vector<uint8_t> Load(const char* name) {
    std::ifstream f(std::string(MAXLIKE_SAMPLES) + "/" + name + ".max", std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
static math::Vec3 Origin(const scene::Node* n) { return math::TransformPoint(n->ObjectMatrix(), {0, 0, 0}); }
static bool Near(const math::Vec3& a, const math::Vec3& b) { return math::Length(a - b) < 1e-9; }

int main() {
    // box + parent_child_link: Box01 clashes -> Box02, still under Sphere01, placed exactly
    // as when the file is opened on its own.
    {
        scene::Scene scene, alone;
        io::ImportReport report;
        std::string error;
        CHECK(io::ImportMax(Load("box"), scene, report, error));
        CHECK(io::ImportMax(Load("parent_child_link"), alone, report, error));
        const math::Vec3 boxBefore = Origin(scene.Find("Box01"));
        std::vector<scene::Node::Id> added;
        CHECK(io::MergeScene(Load("parent_child_link"), scene, report, added, error));
        CHECK(scene.Nodes().size() == 3 && added.size() == 2);
        const scene::Node* sphere = scene.Find("Sphere01");
        const scene::Node* box2 = scene.Find("Box02");
        CHECK(sphere && box2 && box2->Parent() == sphere);
        CHECK(report.notes.size() == 1 && report.notes[0].find("Box02") != std::string::npos);
        CHECK(added.size() == 2 && sphere && added[0] == sphere->GetId());          // parents first
        CHECK(box2 && Near(Origin(box2), Origin(alone.Find("Box01"))));
        CHECK(sphere && Near(Origin(sphere), Origin(alone.Find("Sphere01"))));
        // The undo step: undo removes both merged nodes, redo restores them exactly.
        scene::UndoStack undo;
        const std::shared_ptr<scene::Object> boxObject = box2 ? box2->GetObject() : nullptr;
        undo.Execute(std::make_unique<scene::AddedNodesCommand>("Merge", added), scene);
        CHECK(undo.UndoName() == "Merge");
        CHECK(undo.Undo(scene) && scene.Nodes().size() == 1 && scene.Find("Box01") && !scene.Find("Sphere01"));
        CHECK(Near(Origin(scene.Find("Box01")), boxBefore));
        CHECK(undo.Redo(scene) && scene.Nodes().size() == 3);
        sphere = scene.Find("Sphere01");
        box2 = scene.Find("Box02");
        CHECK(sphere && box2 && box2->Parent() == sphere && box2->GetObject() == boxObject);
        CHECK(box2 && Near(Origin(box2), Origin(alone.Find("Box01"))));
        CHECK(undo.Undo(scene) && scene.Nodes().size() == 1);
    }
    // instances twice: 14 boxes, Box08..Box14, on two shared objects.
    {
        scene::Scene scene;
        io::ImportReport report;
        std::vector<scene::Node::Id> added;
        std::string error;
        CHECK(io::MergeScene(Load("instances"), scene, report, added, error) && report.notes.empty());
        CHECK(io::MergeScene(Load("instances"), scene, report, added, error) && report.notes.size() == 7);
        CHECK(scene.Nodes().size() == 14 && scene.Find("Box14") && scene.Find("Box08"));
        std::set<const scene::Object*> objects;
        for (const auto& n : scene.Nodes()) objects.insert(n->GetObject().get());
        CHECK(objects.size() == 2);
    }
    // A file that cannot be read: false, nothing changed.
    {
        scene::Scene scene;
        io::ImportReport report;
        std::vector<scene::Node::Id> added;
        std::string error;
        CHECK(io::ImportMax(Load("box"), scene, report, error));
        CHECK(!io::MergeScene({1, 2, 3}, scene, report, added, error) && scene.Nodes().size() == 1 && added.empty());
    }
    // Phase 11.2: R3's Merge dialog lists the objects; only the chosen ones come in.
    {
        std::vector<io::MergeCandidate> list;
        std::string error;
        CHECK(io::ListMergeCandidates(Load("instances"), list, error) && list.size() == 7 && list[0].name == "Box01");
        scene::Scene scene;
        io::ImportReport report;
        std::vector<scene::Node::Id> added;
        CHECK(io::MergeChosen(Load("instances"), {0, 2}, scene, report, added, error));
        CHECK(scene.Nodes().size() == 2 && added.size() == 2 && scene.Find("Box01") && scene.Find("Box03") && !scene.Find("Box02"));
        // The child alone: at the top, exactly where it was in the file.
        CHECK(io::ListMergeCandidates(Load("parent_child_link"), list, error) && list.size() == 2);
        int child = -1;
        for (size_t i = 0; i < list.size(); ++i) if (list[i].depth == 1) child = static_cast<int>(i);
        CHECK(child >= 0 && list[static_cast<size_t>(child)].parent >= 0);
        scene::Scene alone, merged;
        CHECK(io::ImportMax(Load("parent_child_link"), alone, report, error));
        CHECK(io::MergeChosen(Load("parent_child_link"), {child}, merged, report, added, error));
        const std::string name = list[static_cast<size_t>(child)].name;
        CHECK(merged.Nodes().size() == 1 && merged.Find(name) && !merged.Find(name)->Parent());
        CHECK(merged.Find(name) && Near(Origin(merged.Find(name)), Origin(alone.Find(name))));
        // Everything chosen: the same as a plain Merge.
        scene::Scene all;
        CHECK(io::MergeChosen(Load("instances"), {0, 1, 2, 3, 4, 5, 6}, all, report, added, error) && all.Nodes().size() == 7);
    }
    if (failures == 0) std::printf("merge: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
