// Tools > Transform Type-In (Phase 9.8): absolute values and offsets, checked by placing the
// results and measuring the world (not by the same formulas), on a linked sample.
#include "io/MaxImport.h"
#include "scene/MoveMath.h"
#include "scene/TypeIn.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(const math::Vec3& a, const math::Vec3& b, double eps = 1e-6) { return math::Length(a - b) < eps; }
using scene::typein::Tool;

int main() {
    // Euler round trip, including all three angles at once.
    for (const math::Vec3 d : {math::Vec3{0, 0, 45}, math::Vec3{30, 0, 0}, math::Vec3{10, -20, 70}, math::Vec3{-35, 15, -120}})
        CHECK(Near(scene::move::EulerXYZDegrees(scene::typein::QuatFromEulerXYZDegrees(d)), d, 1e-9));
    // On parent_child_link's Box01, linked under a rotated Sphere01.
    scene::Scene s;
    {
        std::ifstream f(std::string(MAXLIKE_SAMPLES) + "/parent_child_link.max", std::ios::binary);
        const std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        io::ImportReport r;
        std::string e;
        CHECK(io::ImportMax(b, s, r, e));
    }
    scene::Node* box = s.Find("Box01");
    CHECK(box && box->Parent());
    if (!box) return 1;
    const scene::PRS original = box->Transform();
    auto apply = [&](const scene::PRS& t) { box->Transform() = t; };
    auto origin = [&]() { return math::TransformPoint(box->WorldMatrix(), {0, 0, 0}); };
    // Absolute Move: the pivot lands exactly on the typed world position; the node itself
    // was not changed by computing it.
    {
        const scene::PRS t = scene::typein::WithAbsolute(Tool::Move, *box, {10, 20, 30});
        CHECK(box->Transform().position.x == original.position.x);
        apply(t);
        CHECK(Near(origin(), {10, 20, 30}));
        CHECK(Near(scene::typein::Absolute(Tool::Move, *box), {10, 20, 30}));
        apply(original);
    }
    // Absolute Rotate: the world rotation reads back as typed, the pivot stays put.
    {
        const math::Vec3 before = origin();
        apply(scene::typein::WithAbsolute(Tool::Rotate, *box, {0, 0, 45}));
        CHECK(Near(scene::typein::Absolute(Tool::Rotate, *box), {0, 0, 45}, 1e-6));
        CHECK(Near(origin(), before, 1e-6));
        // A world X axis of the box now points along (cos 45, sin 45, 0).
        const math::Vec3 x = math::TransformPoint(box->WorldMatrix(), {1, 0, 0}) - origin();
        CHECK(Near(math::Normalize(x), {std::cos(3.14159265358979 / 4), std::sin(3.14159265358979 / 4), 0}, 1e-6));
        apply(original);
    }
    // Offsets: Move adds a world vector; Rotate 90 about world Z around a point turns the
    // pivot around it; Scale 200 doubles the scale.
    {
        const math::Vec3 before = origin();
        apply(scene::typein::WithOffset(Tool::Move, *box, {5, 0, -2}, before));
        CHECK(Near(origin(), before + math::Vec3{5, 0, -2}));
        apply(original);
        const math::Vec3 p{0, 0, 0};
        apply(scene::typein::WithOffset(Tool::Rotate, *box, {0, 0, 90}, p));
        CHECK(Near(origin(), {-before.y, before.x, before.z}, 1e-6));
        apply(original);
        apply(scene::typein::WithOffset(Tool::Scale, *box, {200, 200, 200}, before));
        CHECK(Near(box->Transform().scale, original.scale * 2.0));
        apply(original);
        CHECK(Near(scene::typein::Absolute(Tool::Scale, *box), original.scale * 100.0));
    }
    if (failures == 0) std::printf("type-in: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
