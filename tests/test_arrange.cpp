// Tools > Mirror and Tools > Array (Phase 9.10), checked by placing objects and measuring
// the world.
#include "scene/Arrange.h"
#include "scene/Clone.h"
#include "scene/Primitives.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(const math::Vec3& a, const math::Vec3& b, double eps = 1e-6) { return math::Length(a - b) < eps; }
using namespace scene::tools;

int main() {
    scene::Scene s;
    scene::Node* box = s.CreateNode("Box01");
    box->SetObject(std::make_shared<scene::BoxObject>(20, 10, 30));
    box->Transform().position = {10, 5, 0};
    box->Transform().rotation = math::Quat::FromAxisAngle({0, 0, 1}, 0.3);
    const scene::move::Frame world;
    auto worldPoint = [&](const math::Vec3& local) { return math::TransformPoint(box->ObjectMatrix(), local); };
    // Mirror X through the origin: every world point's X is negated; the object now mirrors.
    {
        const math::Vec3 corner = worldPoint({10, 5, 30});
        const scene::PRS before = box->Transform();
        box->Transform() = Changed(*box, MirrorMatrix(world, MirrorAxis::X, {0, 0, 0}, 0));
        CHECK(Near(worldPoint({10, 5, 30}), {-corner.x, corner.y, corner.z}));
        CHECK(math::Mirrors(box->ObjectMatrix()) && box->Transform().scale.x < 0);
        // Twice is the original placement.
        box->Transform() = Changed(*box, MirrorMatrix(world, MirrorAxis::X, {0, 0, 0}, 0));
        CHECK(Near(worldPoint({10, 5, 30}), corner) && !math::Mirrors(box->ObjectMatrix()));
        box->Transform() = before;
        // Through a centre, with an offset along X: x -> 2 c - x + offset.
        box->Transform() = Changed(*box, MirrorMatrix(world, MirrorAxis::X, {50, 0, 0}, 7));
        CHECK(Near(worldPoint({10, 5, 30}), {100 - corner.x + 7, corner.y, corner.z}));
        box->Transform() = before;
        // XY flips X and Y (a half turn: not a mirror).
        box->Transform() = Changed(*box, MirrorMatrix(world, MirrorAxis::XY, {0, 0, 0}, 0));
        CHECK(Near(worldPoint({10, 5, 30}), {-corner.x, -corner.y, corner.z}) && !math::Mirrors(box->ObjectMatrix()));
        box->Transform() = before;
    }
    // Array: step = move 25 along X and turn 90 degrees about Z around the origin. Copy i
    // sits at step^i: the pivot goes (10, 5) -> rotated and shifted, each step.
    {
        ArrayStep step;
        step.move = {25, 0, 0};
        step.rotateDegrees = {0, 0, 90};
        const math::Mat4 m = ArrayStepMatrix(step, world, {0, 0, 0});
        const math::Vec3 p0 = math::TransformPoint(box->WorldMatrix(), {0, 0, 0});
        const math::Vec3 p1 = math::TransformPoint(m, p0);
        CHECK(Near(p1, {-p0.y + 25, p0.x, p0.z}));
        // Scale 200 % about the centre doubles distances from it.
        ArrayStep grow;
        grow.scalePercent = {200, 200, 200};
        CHECK(Near(math::TransformPoint(ArrayStepMatrix(grow, world, {1, 1, 1}), {2, 3, 4}), {3, 5, 7}));
        // Clones placed by the step: three copies 25 apart along X (move only).
        ArrayStep along;
        along.move = {25, 0, 0};
        const math::Mat4 a = ArrayStepMatrix(along, world, {0, 0, 0});
        const auto ids = scene::CloneNodes(s, {box->GetId()}, true);
        scene::Node* copy = s.FindById(ids[0]);
        copy->Transform() = Changed(*copy, a * a);
        CHECK(Near(math::TransformPoint(copy->WorldMatrix(), {0, 0, 0}), p0 + math::Vec3{50, 0, 0}));
        CHECK(copy->GetObject() == box->GetObject());
    }
    if (failures == 0) std::printf("arrange: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
