// Unit tests for creating objects in the scene: the live node while building, one undo
// step per object, R3 naming, colours, cancelling, and the view's construction plane.
#include "scene/CreationTool.h"
#include "scene/Primitives.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(double a, double b) { return std::fabs(a - b) < 1e-9; }

using namespace scene;
using E = Creator::Event;

static void BuildBox(CreationTool& t, int owner, const ConstructionPlane& p) {
    t.Feed(E::Press, owner, p, 0, 0, 300, 1.0);
    t.Feed(E::Move, owner, p, 40, 30, 300, 1.0);
    t.Feed(E::Release, owner, p, 40, 30, 300, 1.0);
    t.Feed(E::Move, owner, p, 40, 30, 280, 1.0);
    t.Feed(E::Press, owner, p, 40, 30, 280, 1.0);          // height 20
}

static void CreateUndoRedo() {
    Scene s;
    UndoStack u;
    CreationTool t(s, u);
    t.SetType(0);                                          // Box
    const uint32_t firstColor = t.Color();
    t.Feed(E::Press, 0, TopPlane(), 0, 0, 300, 1.0);
    CHECK(t.Busy() && s.Nodes().size() == 1 && !u.CanUndo());   // live node, nothing to undo yet
    CHECK(s.Nodes()[0]->Name() == "Box01" && s.Nodes()[0]->WireColor() == firstColor);
    t.Feed(E::Move, 0, TopPlane(), 40, 30, 300, 1.0);
    const auto& live = s.Nodes()[0];
    CHECK(Near(live->GetObject()->Params().Float(BoxObject::kWidth), 40));   // grows live
    CHECK(Near(live->Transform().position.x, 20) && Near(live->Transform().position.y, 15));
    t.Feed(E::Release, 0, TopPlane(), 40, 30, 300, 1.0);
    t.Feed(E::Move, 0, TopPlane(), 40, 30, 280, 1.0);
    t.Feed(E::Press, 0, TopPlane(), 40, 30, 280, 1.0);
    CHECK(!t.Busy() && s.Nodes().size() == 1 && u.UndoCount() == 1 && u.UndoName() == "Create");
    const Node* box = s.Find("Box01");
    CHECK(box && Near(box->GetObject()->Params().Float(BoxObject::kHeight), 20) && box->WireColor() == firstColor);
    CHECK(t.Color() != firstColor && t.LastCreatedName() == "Box01");     // next object: new colour
    // One undo removes it; redo brings it back.
    CHECK(u.Undo(s) && s.Nodes().empty());
    CHECK(u.Redo(s) && s.Find("Box01") && Near(s.Find("Box01")->GetObject()->Params().Float(BoxObject::kHeight), 20));
    // The next box is Box02.
    BuildBox(t, 0, TopPlane());
    CHECK(s.Find("Box02") && u.UndoCount() == 2);
}

static void CancelLeavesNothing() {
    Scene s;
    UndoStack u;
    CreationTool t(s, u);
    t.SetType(2);                                          // Sphere
    t.Feed(E::Press, 0, TopPlane(), 0, 0, 100, 1.0);
    t.Feed(E::Move, 0, TopPlane(), 5, 0, 100, 1.0);
    CHECK(s.Nodes().size() == 1 && t.Cancel());
    CHECK(s.Nodes().empty() && !u.CanUndo() && !t.Busy());
    // A click without a drag: nothing created, nothing to undo.
    t.Feed(E::Press, 0, TopPlane(), 0, 0, 100, 1.0);
    t.Feed(E::Release, 0, TopPlane(), 0, 0, 100, 1.0);
    CHECK(s.Nodes().empty() && !u.CanUndo());
    // Changing the type mid-creation cancels too.
    t.Feed(E::Press, 0, TopPlane(), 0, 0, 100, 1.0);
    t.SetType(0);
    CHECK(s.Nodes().empty() && !t.Busy());
}

static void OwnerAndPlane() {
    Scene s;
    UndoStack u;
    CreationTool t(s, u);
    t.SetType(0);
    t.Feed(E::Press, 1, FrontPlane(), 10, 20, 300, 1.0);  // started in viewport 1 (Front)
    CHECK(!t.Feed(E::Move, 2, TopPlane(), 99, 99, 0, 1.0)); // other viewports are ignored
    t.Feed(E::Move, 1, FrontPlane(), 30, 40, 300, 1.0);
    const Node* n = s.Nodes()[0].get();
    // Front plane: the base centre (20, 30) in (u, v) = (X, Z); the box faces the viewer.
    CHECK(Near(n->Transform().position.x, 20) && Near(n->Transform().position.z, 30) && Near(n->Transform().position.y, 0));
    const math::Vec3 z = n->Transform().rotation.Rotate({0, 0, 1});
    CHECK(Near(z.y, -1));
    t.Cancel();
    // No type: events do nothing.
    t.SetType(-1);
    CHECK(!t.Feed(E::Press, 0, TopPlane(), 0, 0, 0, 1.0) && s.Nodes().empty());
}

static void EveryTypeCreates() {
    for (int type = 0; type < 10; ++type) {
        Scene s;
        UndoStack u;
        CreationTool t(s, u);
        t.SetType(type);
        // Enough steps for any primitive: a drag, then up to two clicks.
        t.Feed(E::Press, 0, TopPlane(), 0, 0, 300, 1.0);
        t.Feed(E::Release, 0, TopPlane(), 20, 0, 300, 1.0);
        for (int i = 0; i < 2 && t.Busy(); ++i) {
            t.Feed(E::Move, 0, TopPlane(), 10, 0, 280 - 10 * i, 1.0);
            t.Feed(E::Press, 0, TopPlane(), 10, 0, 280 - 10 * i, 1.0);
        }
        CHECK(!t.Busy() && s.Nodes().size() == 1 && u.UndoCount() == 1);
        CHECK(s.Nodes()[0]->Name() == std::string(PrimitiveBaseName(type)) + "01");
        CHECK(!s.Nodes()[0]->GetObject()->Evaluate().faces.empty());   // a real mesh
    }
}

static void KeyboardEntry() {
    Scene s;
    UndoStack u;
    CreationTool t(s, u);
    CHECK(t.CreateDirect({}, TopPlane(), {0, 0, 0}).empty());       // no type selected
    t.SetType(0);                                                  // Box
    const uint32_t c = t.Color();
    const std::string name = t.CreateDirect({{BoxObject::kLength, 30}, {BoxObject::kWidth, 20}, {BoxObject::kHeight, 10}},
                                            FrontPlane(), {5, 6, 7});
    const Node* n = s.Find(name);
    CHECK(name == "Box01" && n && n->WireColor() == c && t.Color() != c);
    CHECK(Near(n->GetObject()->Params().Float(BoxObject::kHeight), 10));
    CHECK(Near(n->Transform().position.x, 5) && Near(n->Transform().position.y, 6) && Near(n->Transform().position.z, 7));
    CHECK(Near(n->Transform().rotation.Rotate({0, 0, 1}).y, -1));   // oriented like the Front view
    CHECK(u.UndoCount() == 1 && u.Undo(s) && s.Nodes().empty());    // one undo step
}

int main() {
    KeyboardEntry();
    CreateUndoRedo();
    CancelLeavesNothing();
    OwnerAndPlane();
    EveryTypeCreates();
    if (failures == 0) std::printf("creation tool: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
