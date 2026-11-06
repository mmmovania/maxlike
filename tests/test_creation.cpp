// Unit tests for interactive creation: the step sequences, live parameters, cancelling,
// and the construction planes of the views.
#include "scene/Creation.h"
#include "scene/Primitives.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(double a, double b) { return std::fabs(a - b) < 1e-9; }
static bool Near(const math::Vec3& a, const math::Vec3& b) { return Near(a.x, b.x) && Near(a.y, b.y) && Near(a.z, b.z); }

using namespace scene;
using E = Creator::Event;

static double ParamOf(const Creator& c, size_t index) {
    for (const auto& [i, v] : c.Params()) if (i == index) return v;
    return NAN;
}

static void BoxTwoSteps() {
    Creator c(Primitive::Box, 0.5);                       // 0.5 world units per pixel
    c.Feed(E::Press, 10, 20, 300);                         // corner at (10, 20)
    c.Feed(E::Move, -30, 50, 300);
    CHECK(c.Started() && !c.Done() && c.Step() == 0);
    CHECK(Near(ParamOf(c, BoxObject::kWidth), 40) && Near(ParamOf(c, BoxObject::kLength), 30));
    CHECK(Near(c.CenterU(), -10) && Near(c.CenterV(), 35));   // pivot at the base's centre
    c.Feed(E::Release, -30, 50, 300);
    CHECK(c.Step() == 1 && !c.Done());
    c.Feed(E::Move, -30, 50, 260);                         // 40 px up = +20 units
    CHECK(Near(ParamOf(c, BoxObject::kHeight), 20));
    c.Feed(E::Move, -30, 50, 340);                         // below the start: negative height
    CHECK(Near(ParamOf(c, BoxObject::kHeight), -20));
    c.Feed(E::Press, -30, 50, 280);
    CHECK(c.Done() && Near(ParamOf(c, BoxObject::kHeight), 10));
    c.Feed(E::Move, 0, 0, 0);                              // ignored once done
    CHECK(Near(ParamOf(c, BoxObject::kHeight), 10));
}

static void ClickWithoutDragCancels() {
    Creator c(Primitive::Box, 1.0);
    c.Feed(E::Press, 5, 5, 100);
    c.Feed(E::Release, 5.5, 5, 100);                       // half a pixel: not a drag
    CHECK(c.Cancelled() && !c.Done());
    Creator s(Primitive::Sphere, 1.0);
    s.Feed(E::Press, 0, 0, 100);
    s.Feed(E::Move, 3, 4, 100);
    s.Cancel();                                            // right-click
    CHECK(s.Cancelled());
}

static void SphereOneDrag() {
    Creator c(Primitive::Sphere, 1.0);
    c.Feed(E::Move, 1, 1, 0);                              // moves before the press are ignored
    CHECK(!c.Started());
    c.Feed(E::Press, 2, 3, 100);
    c.Feed(E::Move, 5, 7, 100);
    c.Feed(E::Release, 5, 7, 100);
    CHECK(c.Done() && Near(ParamOf(c, SphereObject::kRadius), 5));
    CHECK(Near(c.CenterU(), 2) && Near(c.CenterV(), 3));   // centred on the press point
}

static void ConeThreeSteps() {
    Creator c(Primitive::Cone, 1.0);
    c.Feed(E::Press, 0, 0, 200);
    c.Feed(E::Release, 10, 0, 200);
    CHECK(c.Step() == 1 && Near(ParamOf(c, ConeObject::kRadius1), 10) && Near(ParamOf(c, ConeObject::kRadius2), 0));
    c.Feed(E::Move, 10, 0, 170);
    c.Feed(E::Press, 10, 0, 170);                          // height 30
    CHECK(c.Step() == 2 && Near(ParamOf(c, ConeObject::kHeight), 30));
    c.Feed(E::Move, 4, 0, 170);
    c.Feed(E::Press, 4, 0, 170);                           // radius 2 = 4
    CHECK(c.Done() && Near(ParamOf(c, ConeObject::kRadius2), 4) && Near(ParamOf(c, ConeObject::kHeight), 30));
}

static void TubeAndTorus() {
    Creator t(Primitive::Tube, 1.0);
    t.Feed(E::Press, 0, 0, 200);
    t.Feed(E::Move, 12, 0, 200);
    CHECK(Near(ParamOf(t, TubeObject::kRadius2), 12));       // follows radius 1 until its step
    t.Feed(E::Release, 12, 0, 200);
    t.Feed(E::Move, 8, 0, 200);
    t.Feed(E::Press, 8, 0, 200);
    t.Feed(E::Move, 8, 0, 190);
    t.Feed(E::Press, 8, 0, 190);
    CHECK(t.Done() && Near(ParamOf(t, TubeObject::kRadius1), 12) && Near(ParamOf(t, TubeObject::kRadius2), 8) &&
          Near(ParamOf(t, TubeObject::kHeight), 10));
    Creator r(Primitive::Torus, 1.0);
    r.Feed(E::Press, 0, 0, 0);
    r.Feed(E::Release, 20, 0, 0);
    r.Feed(E::Move, 23, 0, 0);                             // 3 beyond the ring
    r.Feed(E::Press, 23, 0, 0);
    CHECK(r.Done() && Near(ParamOf(r, TorusObject::kRadius1), 20) && Near(ParamOf(r, TorusObject::kRadius2), 3));
}

static void PlaneAndPyramid() {
    Creator p(Primitive::Plane, 1.0);
    p.Feed(E::Press, 0, 0, 0);
    p.Feed(E::Release, 40, -42, 0);
    CHECK(p.Done() && Near(ParamOf(p, PlaneObject::kWidth), 40) && Near(ParamOf(p, PlaneObject::kLength), 42));
    Creator y(Primitive::Pyramid, 1.0);
    y.Feed(E::Press, 0, 0, 100);
    y.Feed(E::Release, 20, 20, 100);
    y.Feed(E::Press, 20, 20, 90);
    CHECK(y.Done() && Near(ParamOf(y, PyramidObject::kHeight), 10) && Near(ParamOf(y, PyramidObject::kDepth), 20));
}

static void Planes() {
    // Every plane is orthonormal and right-handed (u x v = normal).
    for (const ConstructionPlane& p : {TopPlane(), FrontPlane(), LeftPlane()}) {
        CHECK(Near(math::Cross(p.u, p.v), p.normal));
        CHECK(Near(math::Length(p.u), 1) && Near(math::Dot(p.u, p.v), 0));
    }
    // Front: an object created there stands with its local Z towards the viewer (-Y),
    // its local Y up (+Z); the pivot goes where (u, v) points.
    const PRS f = FrontPlane().NodeTransform(5, 7);
    CHECK(Near(f.position, {5, 0, 7}));
    CHECK(Near(f.rotation.Rotate({0, 0, 1}), {0, -1, 0}) && Near(f.rotation.Rotate({0, 1, 0}), {0, 0, 1}));
    // Left: local X runs along -Y (the view's right), local Z towards the viewer (-X).
    const PRS l = LeftPlane().NodeTransform(2, 3);
    CHECK(Near(l.position, {0, -2, 3}) && Near(l.rotation.Rotate({1, 0, 0}), {0, -1, 0}));
    CHECK(Near(l.rotation.Rotate({0, 0, 1}), {-1, 0, 0}));
    // Top: no rotation.
    const PRS t = TopPlane().NodeTransform(1, 2);
    CHECK(Near(t.position, {1, 2, 0}) && Near(t.rotation.w, 1));
}

int main() {
    BoxTwoSteps();
    ClickWithoutDragCancels();
    SphereOneDrag();
    ConeThreeSteps();
    TubeAndTorus();
    PlaneAndPyramid();
    Planes();
    if (failures == 0) std::printf("creation: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
