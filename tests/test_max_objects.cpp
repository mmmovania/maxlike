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
    if (failures == 0) std::printf("max objects: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
