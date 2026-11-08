// io::ReadNodes against the R3.1 samples (docs/reference/max-format.md §4–§5). Matrices
// are checked against positions computed independently (Quat::Rotate and vectors).
#include "io/MaxNodes.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static std::vector<io::MaxNode> Nodes(const char* name, io::MaxScene& s) {
    std::ifstream f(std::string(MAXLIKE_SAMPLES) + "/" + name + ".max", std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::string error;
    if (!s.Open(b, error)) std::printf("  %s: %s\n", name, error.c_str());
    return io::ReadNodes(s);
}
static bool Near(double a, double b, double eps = 1e-3) { return std::fabs(a - b) < eps; }
static bool Near(const math::Vec3& a, const math::Vec3& b, double eps = 1e-3) { return Near(a.x, b.x, eps) && Near(a.y, b.y, eps) && Near(a.z, b.z, eps); }
static const double kDeg = 3.14159265358979323846 / 180.0;

int main() {
    // box.max: one node at the origin under the root, teal, identity transform.
    {
        io::MaxScene s;
        const auto n = Nodes("box", s);
        CHECK(n.size() == 1 && n[0].name == "Box01" && n[0].parent == -1 && n[0].index == 275);
        CHECK(n[0].hasWireColor && n[0].wire[0] == 26 && n[0].wire[1] == 177 && n[0].wire[2] == 148);   // 0x1A, 0xB1, 0x94
        CHECK(n[0].controller == 271 && n[0].object == 273 && n[0].notes.empty());
        CHECK(Near(n[0].transform.position, {0, 0, 0}) && Near(n[0].transform.scale, {1, 1, 1}));
    }
    // pivot.max: the first non-identity object offset; the geometry sits at the offset
    // from the pivot, turned with the node.
    {
        io::MaxScene s;
        const auto n = Nodes("pivot", s);
        CHECK(n.size() == 1 && n[0].notes.empty());
        // Equal and opposite: the pivot was moved up by 20.16222, half the Box's height
        // (40.3244, read in 8.4): R3's "Center to Object".
        CHECK(Near(n[0].objectOffset.position, {0, 0, -20.16222}, 1e-4));
        CHECK(Near(n[0].transform.position, {-0.9269981, 0.4634991, 20.16222}, 1e-4));
        const math::Vec3 pivot = math::TransformPoint(io::WorldMatrix(n, 0), {0, 0, 0});
        CHECK(Near(pivot, n[0].transform.position));
        const math::Vec3 origin = math::TransformPoint(io::ObjectMatrix(n, 0), {0, 0, 0});
        CHECK(Near(origin, n[0].transform.position + n[0].transform.rotation.Rotate(n[0].objectOffset.position)));
        CHECK(!Near(origin, pivot, 1.0));                                       // the offset really applies
    }
    // parent_child_link.max: Box01 is linked to Sphere01; its world position is the
    // sphere's transform applied to its own.
    {
        io::MaxScene s;
        const auto n = Nodes("parent_child_link", s);
        CHECK(n.size() == 2 && n[0].name == "Sphere01" && n[1].name == "Box01");
        CHECK(n[0].parent == -1 && n[1].parent == 0);
        const math::Vec3 expected = n[0].transform.position + n[0].transform.rotation.Rotate(n[1].transform.position);
        CHECK(Near(math::TransformPoint(io::WorldMatrix(n, 1), {0, 0, 0}), expected));
        CHECK(!Near(expected, n[1].transform.position, 1.0));                   // the parent really applies
    }
    // instances.max: the handedness (§5.2): Box01 stores (0, 0, 0.3543, 0.9351) and R3's
    // Top view shows it turned -41.5 degrees; Box07 the other way.
    {
        io::MaxScene s;
        const auto n = Nodes("instances", s);
        CHECK(n.size() == 7 && n[0].name == "Box01" && n[6].name == "Box07");
        CHECK(Near(n[0].transform.rotation.AngleAbout({0, 0, 1}), -41.5 * kDeg, 0.1 * kDeg));
        CHECK(Near(n[6].transform.rotation.AngleAbout({0, 0, 1}), 2 * std::asin(0.3987), 0.001));      // +47.0 degrees
        for (const auto& node : n) CHECK(node.object == 275);                   // all on one Derived Object
    }
    // boxes.max: a non-uniform scale; equal wire colours.
    {
        io::MaxScene s;
        const auto n = Nodes("boxes", s);
        CHECK(n.size() == 4 && n[3].name == "Box04");
        CHECK(Near(n[3].transform.scale, {0.6061, 1.45, 1}, 0.0005));
        const math::Vec3 corner = math::TransformPoint(io::WorldMatrix(n, 3), {1, 1, 0});
        CHECK(Near(corner, n[3].transform.position + math::Vec3{0.6061, 1.45, 0}, 0.0005));   // no rotation on Box04
        for (const auto& node : n) CHECK(node.wire[0] == 108 && node.wire[1] == 8 && node.wire[2] == 136);
    }
    // Every sample: a name, a valid object, a valid parent, nothing unread.
    for (const char* f : {"box", "box_editmesh", "boxes", "cone", "cylinder", "geosphere", "instances", "parent_child_link",
                          "pivot", "plane", "pyramid", "sphere", "teapot", "teapot_edit_mesh", "teapot_smoothinggroups",
                          "torus", "tube"}) {
        io::MaxScene s;
        const auto n = Nodes(f, s);
        CHECK(!n.empty());
        for (const auto& node : n) {
            CHECK(!node.name.empty() && node.object >= 0 && static_cast<size_t>(node.object) < s.ObjectCount());
            CHECK(node.parent >= -1 && node.parent < static_cast<int>(n.size()));
            CHECK(node.notes.empty());
            if (!node.notes.empty()) std::printf("  %s %s: %s\n", f, node.name.c_str(), node.notes[0].c_str());
        }
    }
    if (failures == 0) std::printf("max nodes: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
