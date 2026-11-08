// io::MaxScene against the R3.1 samples (docs/reference/samples): the class and DLL
// directories (class IDs as in the 3ds Max SDK), the scene list, and references from
// both chunk forms (docs/reference/max-format.md §2–§3).
#include "io/MaxScene.h"
#include <cstdio>
#include <fstream>
#include <iterator>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static bool Open(const char* name, io::MaxScene& s) {
    std::ifstream f(std::string(MAXLIKE_SAMPLES) + "/" + name + ".max", std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::string error;
    const bool ok = s.Open(b, error);
    if (!ok) std::printf("  %s: %s\n", name, error.c_str());
    return ok;
}

int main() {
    // box.max in detail.
    {
        io::MaxScene s;
        CHECK(Open("box", s));
        CHECK(s.Classes().size() == 35 && s.Dlls().size() == 5 && s.ObjectCount() == 299);
        const auto& box = s.Classes()[14];
        CHECK(box.name == "Box" && box.classIdA == 0x10 && box.classIdB == 0 && box.superClassId == 0x10);   // BOXOBJ_CLASS_ID, GEOMOBJECT
        CHECK(box.dll == 2 && s.Dlls()[2].file == "prim.dlo");                                                // Max's primitives plug-in
        const auto& prs = s.Classes()[13];
        CHECK(prs.name == "Position/Rotation/Scale" && prs.classIdA == 0x2005 && prs.superClassId == 0x9008 && prs.dll == -1);
        CHECK(s.Classes()[16].name == "Node" && s.Classes()[16].classIdA == 1 && s.Classes()[16].superClassId == 1);
        CHECK(s.Dlls()[0].file == "mtl.dlt" && !s.Dlls()[0].description.empty());                            // 0x2037 = file name
        // The node (#275): 8 slots from chunk 0x2035; slot 0 its controller, 1 its object, 6 its layer.
        CHECK(s.FindByClassName("Node") == std::vector<size_t>{275});
        CHECK(s.Refs(275) == (std::vector<int32_t>{271, 273, -1, -1, -1, -1, 274, -1}));
        CHECK(s.ClassOf(271)->name == "Position/Rotation/Scale" && s.ClassOf(273)->name == "Box" && s.ClassOf(274)->name == "Base Layer");
        // The controller's positional references (chunk 0x2034); the Box's parameter block.
        CHECK(s.Refs(271) == (std::vector<int32_t>{268, 269, 270}));
        CHECK(s.ClassOf(268)->name == "Bezier Position" && s.ClassOf(269)->name == "TCB Rotation" && s.ClassOf(270)->name == "Bezier Scale");
        CHECK(s.Refs(273) == std::vector<int32_t>{272} && s.ClassOf(272)->name == "ParamBlock");
        // A 0x2035 with slots but no filled ones: two empty slots.
        CHECK(s.Refs(268) == (std::vector<int32_t>{-1, -1}));
    }
    // instances.max: seven nodes sharing one Derived Object (Edit Mesh over a Box).
    {
        io::MaxScene s;
        CHECK(Open("instances", s));
        const auto nodes = s.FindByClassName("Node");
        CHECK(nodes.size() == 7);
        for (size_t n : nodes) CHECK(s.Refs(n).size() > 1 && s.Refs(n)[1] == 275);
        CHECK(s.IsDerivedObject(275) && s.Type(275) == 0x2032 && s.ClassOf(275) == nullptr);
        CHECK(s.Refs(275) == (std::vector<int32_t>{272, 274}));
        CHECK(s.ClassOf(272)->name == "Edit Mesh" && s.ClassOf(274)->name == "Box");
    }
    // Every sample: references inside the scene list, valid DLL indices, every type known;
    // the geometry each holds.
    struct Expect { const char* file; size_t nodes; const char* geometry; };
    const Expect all[] = {{"box", 1, "Box"}, {"box_editmesh", 1, "Editable Mesh"}, {"boxes", 4, "Box"}, {"cone", 1, "Cone"},
                          {"cylinder", 1, "Cylinder"}, {"geosphere", 1, "GeoSphere"}, {"instances", 7, "Box"},
                          {"parent_child_link", 2, "Box"}, {"pivot", 1, "Box"}, {"plane", 1, "Plane"}, {"pyramid", 1, "Pyramid"},
                          {"sphere", 1, "Sphere"}, {"teapot", 1, "Teapot"}, {"teapot_edit_mesh", 1, "Editable Mesh"},
                          {"teapot_smoothinggroups", 1, "Editable Mesh"}, {"torus", 1, "Torus"}, {"tube", 1, "Tube"}};
    for (const Expect& e : all) {
        io::MaxScene s;
        CHECK(Open(e.file, s));
        CHECK(s.FindByClassName("Node").size() == e.nodes);
        CHECK(!s.FindByClassName(e.geometry).empty());
        for (size_t i = 0; i < s.ObjectCount(); ++i) {
            CHECK(s.ClassOf(i) != nullptr || s.IsDerivedObject(i));
            for (int32_t r : s.Refs(i)) CHECK(r >= -1 && r < static_cast<int32_t>(s.ObjectCount()));
        }
        for (const auto& c : s.Classes()) CHECK(c.dll >= -1 && c.dll < static_cast<int32_t>(s.Dlls().size()));
    }
    // Not a .max file: an error, not a crash.
    {
        io::MaxScene s;
        std::string error;
        CHECK(!s.Open({1, 2, 3}, error) && !error.empty());
    }
    if (failures == 0) std::printf("max scene: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
