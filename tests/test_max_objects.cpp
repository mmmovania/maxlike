// io::ReadParamBlock and io::ReadPrimitive against the R3.1 samples. Each imported
// primitive, built by maxlike's own code, must give the vertex and face counts R3.1's
// Summary Info reported for that file (docs/reference/max-format.md §7.6).
#include "io/MaxObjects.h"
#include "scene/Primitives.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static bool Open(const char* name, io::MaxScene& s) {
    std::ifstream f(std::string(MAXLIKE_SAMPLES) + "/" + name + ".max", std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::string error;
    return s.Open(b, error);
}

// The first geometry object of a sample, imported.
static std::shared_ptr<scene::Object> Import(const char* name, std::vector<std::string>& notes) {
    io::MaxScene s;
    if (!Open(name, s)) return nullptr;
    for (size_t i = 0; i < s.ObjectCount(); ++i)
        if (const io::ClassEntry* c = s.ClassOf(i); c && c->superClassId == 0x10) return io::ReadPrimitive(s, i, notes);
    return nullptr;
}

int main() {
    // R3.1's own counts (File > Summary Info) for the samples that have them.
    struct Count { const char* file; int type; size_t verts, faces; };
    const Count counts[] = {{"cone", 1, 146, 288}, {"tube", 5, 216, 432}, {"cylinder", 4, 110, 216}, {"torus", 6, 288, 576},
                            {"sphere", 2, 482, 960}, {"geosphere", 3, 162, 320}, {"teapot", 8, 530, 1024}};
    for (const Count& c : counts) {
        std::vector<std::string> notes;
        const auto obj = Import(c.file, notes);
        CHECK(obj != nullptr);
        if (!obj) continue;
        const scene::Mesh m = obj->Evaluate();
        CHECK(m.verts.size() == c.verts && m.FaceCount() == c.faces);
        if (m.verts.size() != c.verts || m.FaceCount() != c.faces)
            std::printf("  %s: %zu / %zu, R3.1: %zu / %zu\n", c.file, m.verts.size(), m.FaceCount(), c.verts, c.faces);
        CHECK(notes.empty());
    }
    // Values, both block formats.
    {
        std::vector<std::string> notes;
        auto box = Import("box", notes);
        CHECK(box && box->Params().Float(scene::BoxObject::kLength) == 60 && box->Params().Float(scene::BoxObject::kHeight) == 60);
        CHECK(box && box->Params().Int(scene::BoxObject::kWidthSegs) == 1 && box->Evaluate().verts.size() == 8 && box->Evaluate().FaceCount() == 12);
        auto plane = Import("plane", notes);                                     // ParamBlock2
        CHECK(plane && plane->Params().Float(scene::PlaneObject::kLength) == 42 && plane->Params().Float(scene::PlaneObject::kWidth) == 40);
        CHECK(plane && plane->Params().Int(scene::PlaneObject::kLengthSegs) == 4 && plane->Params().Int(scene::PlaneObject::kWidthSegs) == 4);
        CHECK(plane && plane->Evaluate().verts.size() == 25 && plane->Evaluate().FaceCount() == 32);
        auto geo = Import("geosphere", notes);                                   // ParamBlock2, entries out of id order
        CHECK(geo && geo->Params().Int(scene::GeoSphereObject::kBaseType) == 2 && geo->Params().Int(scene::GeoSphereObject::kSegments) == 4);
        auto pyramid = Import("pyramid", notes);
        CHECK(pyramid && pyramid->Params().Float(scene::PyramidObject::kWidth) == 20 && pyramid->Params().Float(scene::PyramidObject::kHeight) == 10);
        auto tube = Import("tube", notes);                                       // Tube's own order: sides before cap segments
        CHECK(tube && tube->Params().Int(scene::TubeObject::kSides) == 18 && tube->Params().Int(scene::TubeObject::kHeightSegs) == 5);
        CHECK(notes.empty());
    }
    // The Cone stores radius 2 = -20; Max builds it as 0 (§6.1), and so do we.
    {
        io::MaxScene s;
        CHECK(Open("cone", s));
        std::vector<io::MaxParam> params;
        std::vector<std::string> notes;
        CHECK(io::ReadParamBlock(s, 272, params, notes) && params.size() == 11);
        CHECK(params.size() > 1 && params[1].index == 1 && params[1].value == -20);
        auto cone = Import("cone", notes);
        CHECK(cone && cone->Params().Float(scene::ConeObject::kRadius2) == 0);
    }
    // pivot.max: the object offset (8.3) is minus half the Box's height: the pivot was
    // moved to the box's centre.
    {
        std::vector<std::string> notes;
        auto box = Import("pivot", notes);
        CHECK(box && std::fabs(box->Params().Float(scene::BoxObject::kHeight) / 2 - 20.16222) < 1e-4);
    }
    // Not a primitive, or not a parameter block: null and a note, no crash.
    {
        io::MaxScene s;
        CHECK(Open("box", s));
        std::vector<std::string> notes;
        CHECK(io::ReadPrimitive(s, 275, notes) == nullptr && !notes.empty());   // the node
        std::vector<io::MaxParam> params;
        notes.clear();
        CHECK(!io::ReadParamBlock(s, 273, params, notes) && !notes.empty());     // the Box itself
    }
    // Editable Mesh: R3.1's collapsed Box equals maxlike's 60-unit Box in every field
    // (positions, faces with winding, smoothing groups, material IDs, edge visibility).
    {
        io::MaxScene s;
        CHECK(Open("box_editmesh", s));
        std::vector<std::string> notes;
        const auto obj = io::ReadEditableMesh(s, s.FindByClassName("Editable Mesh")[0], notes);
        CHECK(obj && notes.empty() && obj->ClassName() == "Editable Mesh");
        scene::BoxObject box;
        for (int i : {0, 1, 2}) box.Params().Set(static_cast<size_t>(i), 60);
        const scene::Mesh a = obj ? obj->Evaluate() : scene::Mesh{}, b = box.Evaluate();
        CHECK(a.verts.size() == 8 && a.FaceCount() == 12 && b.verts.size() == 8 && b.FaceCount() == 12);
        for (size_t i = 0; i < a.verts.size() && i < b.verts.size(); ++i) CHECK(math::Length(a.verts[i] - b.verts[i]) < 1e-4);
        for (size_t i = 0; i < a.faces.size() && i < b.faces.size(); ++i) {
            const auto &f = a.faces[i], &g = b.faces[i];
            CHECK(f.v[0] == g.v[0] && f.v[1] == g.v[1] && f.v[2] == g.v[2]);
            CHECK(f.smoothing == g.smoothing && f.materialId == g.materialId && f.edgeVisible == g.edgeVisible);
        }
        CHECK(a.FaceCount() == 12 && a.faces[0].materialId == 1 && a.faces[2].materialId == 0 && a.faces[0].edgeVisible == 0x3);
    }
    // R3.1's collapsed teapot (radius 30, 4 segments) and maxlike's: the same 530 points
    // (with Max's control point 16, TeapotData.h) and the same counts.
    {
        io::MaxScene s;
        CHECK(Open("teapot_edit_mesh", s));
        std::vector<std::string> notes;
        const auto obj = io::ReadEditableMesh(s, s.FindByClassName("Editable Mesh")[0], notes);
        scene::TeapotObject teapot;
        teapot.Params().Set(scene::TeapotObject::kRadius, 30);
        teapot.Params().Set(scene::TeapotObject::kSegments, 4);
        const scene::Mesh a = obj ? obj->Evaluate() : scene::Mesh{}, b = teapot.Evaluate();
        CHECK(a.verts.size() == 530 && a.FaceCount() == 1024 && b.verts.size() == 530 && b.FaceCount() == 1024);
        double worst = 0;
        for (const auto& p : a.verts) {
            double best = 1e9;
            for (const auto& q : b.verts) best = std::min(best, math::Length(p - q));
            worst = std::max(worst, best);
        }
        CHECK(worst < 1e-3);
        if (worst >= 1e-3) std::printf("  teapot: a Max point is %.4f from maxlike's nearest\n", worst);
    }
    // teapot_smoothinggroups: 56 faces made flat (smoothing 0), the rest in group 1.
    {
        io::MaxScene s;
        CHECK(Open("teapot_smoothinggroups", s));
        std::vector<std::string> notes;
        const auto obj = io::ReadEditableMesh(s, s.FindByClassName("Editable Mesh")[0], notes);
        size_t flat = 0, one = 0;
        if (obj) for (const auto& f : obj->Evaluate().faces) { flat += f.smoothing == 0; one += f.smoothing == 1; }
        CHECK(flat == 56 && one == 968);
    }
    // instances.max: seven nodes, one Derived Object (Edit Mesh over a Box). Read once and
    // shared; the Edit Mesh's moves turn the top four vertices rigidly by +25 degrees.
    {
        io::MaxScene s;
        CHECK(Open("instances", s));
        io::ObjectReader reader(s);
        std::shared_ptr<scene::Object> first;
        for (size_t n : s.FindByClassName("Node")) {
            const auto obj = reader.Read(static_cast<size_t>(s.Refs(n)[1]));
            CHECK(obj != nullptr);
            if (!first) first = obj;
            CHECK(obj == first);                                                 // one object, shared
        }
        CHECK(reader.Notes().empty());
        const auto derived = std::dynamic_pointer_cast<scene::DerivedObject>(first);
        CHECK(derived && derived->Modifiers().size() == 1 && derived->Base()->ClassName() == "Box");
        const auto em = derived ? std::dynamic_pointer_cast<scene::EditMeshModifier>(derived->Modifiers()[0]) : nullptr;
        CHECK(em && em->Moves().size() == 4);
        if (derived) {
            const scene::Mesh before = derived->EvaluateUpTo(0), after = derived->Evaluate();
            for (size_t v = 4; v < 8; ++v) {
                const double a0 = std::atan2(before.verts[v].y, before.verts[v].x), a1 = std::atan2(after.verts[v].y, after.verts[v].x);
                double turn = (a1 - a0) * 180 / 3.14159265358979323846;
                if (turn < -180) turn += 360;
                if (turn > 180) turn -= 360;
                CHECK(std::fabs(turn - 25) < 0.01);
                CHECK(std::fabs(std::hypot(after.verts[v].x, after.verts[v].y) - std::hypot(before.verts[v].x, before.verts[v].y)) < 1e-3);
            }
            for (size_t v = 0; v < 4; ++v) CHECK(math::Length(after.verts[v] - before.verts[v]) < 1e-6);   // the bottom stays
        }
    }
    // Every node of every sample reads without a note.
    for (const char* f : {"box", "box_editmesh", "boxes", "cone", "cylinder", "geosphere", "instances", "parent_child_link",
                          "pivot", "plane", "pyramid", "sphere", "teapot", "teapot_edit_mesh", "teapot_smoothinggroups",
                          "torus", "tube"}) {
        io::MaxScene s;
        CHECK(Open(f, s));
        io::ObjectReader reader(s);
        for (size_t n : s.FindByClassName("Node")) CHECK(reader.Read(static_cast<size_t>(s.Refs(n)[1])) != nullptr);
        CHECK(reader.Notes().empty());
        for (const auto& note : reader.Notes()) std::printf("  %s: %s\n", f, note.c_str());
    }
    if (failures == 0) std::printf("max objects: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
