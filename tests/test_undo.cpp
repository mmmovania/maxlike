// Unit tests for undo/redo: every command returns the scene exactly to its previous
// state, ids survive delete/recreate, and the stack behaves like Max's.
#include "math/Quat.h"
#include "scene/Primitives.h"
#include "scene/Undo.h"
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Same(const scene::PRS& a, const scene::PRS& b) {
    auto eq = [](double x, double y) { return std::fabs(x - y) < 1e-12; };
    return eq(a.position.x, b.position.x) && eq(a.position.y, b.position.y) && eq(a.position.z, b.position.z) &&
           eq(a.rotation.x, b.rotation.x) && eq(a.rotation.y, b.rotation.y) && eq(a.rotation.z, b.rotation.z) &&
           eq(a.rotation.w, b.rotation.w) && eq(a.scale.x, b.scale.x) && eq(a.scale.y, b.scale.y) && eq(a.scale.z, b.scale.z);
}
static scene::PRS At(double x, double y, double z, double deg = 0) {
    scene::PRS t;
    t.position = {x, y, z};
    t.rotation = math::Quat::FromAxisAngle({0, 0, 1}, deg * 3.14159265358979323846 / 180);
    return t;
}

using namespace scene;

static void CreateUndoRedo() {
    Scene s;
    UndoStack u;
    auto box = std::make_shared<BoxObject>(10, 20, 30);
    auto cmd = std::make_unique<CreateNodeCommand>("Box01", box, 0x1AB194, At(5, 6, 7, 30));
    CreateNodeCommand* create = cmd.get();
    u.Execute(std::move(cmd), s);
    const Node::Id id = create->CreatedId();
    CHECK(s.Nodes().size() == 1 && s.FindById(id) && u.UndoName() == "Create");
    CHECK(u.Undo(s) && s.Nodes().empty() && u.CanRedo() && !u.CanUndo());
    CHECK(u.Redo(s) && s.Nodes().size() == 1);
    Node* again = s.FindById(id);                          // same id after redo
    CHECK(again && again->Name() == "Box01" && again->GetObject() == box && again->WireColor() == 0x1AB194);
    CHECK(Same(again->Transform(), At(5, 6, 7, 30)));
    // New ids are never reused, even after undo/redo.
    CHECK(s.CreateNode("Other")->GetId() > id);
}

static void DeleteRestoresHierarchyExactly() {
    Scene s;
    UndoStack u;
    Node* grand = s.CreateNode("Grand");
    Node* parent = s.CreateNode("Parent");
    Node* child = s.CreateNode("Child");
    grand->Transform() = At(100, 0, 0, 90);
    parent->Transform() = At(10, 0, 0, 30);
    child->Transform() = At(3, 4, 5, -20);
    parent->ObjectOffset().position = {1, 2, 3};
    s.Link(parent, grand);
    s.Link(child, parent);
    const PRS parentLocal = parent->Transform(), childLocal = child->Transform();
    const Node::Id gid = grand->GetId(), pid = parent->GetId(), cid = child->GetId();

    u.Execute(std::make_unique<DeleteNodeCommand>(pid), s);
    CHECK(!s.FindById(pid) && s.FindById(cid)->Parent() == s.FindById(gid));   // child moved up
    CHECK(u.Undo(s));
    Node* p = s.FindById(pid);
    Node* c = s.FindById(cid);
    CHECK(p && c && p->Parent() == s.FindById(gid) && c->Parent() == p);
    CHECK(Same(p->Transform(), parentLocal) && Same(c->Transform(), childLocal));   // exact, not re-solved
    CHECK(p->ObjectOffset().position.z == 3 && p->Name() == "Parent");
    CHECK(u.Redo(s) && !s.FindById(pid));
}

static void TransformParamLinkRenameColor() {
    Scene s;
    UndoStack u;
    auto box = std::make_shared<BoxObject>(10, 10, 10);
    Node* a = s.CreateNode("A");
    Node* b = s.CreateNode("B");
    a->SetObject(box);
    b->SetObject(box);                                     // instances
    const PRS before = a->Transform(), after = At(50, 0, 0, 45);
    u.Execute(std::make_unique<TransformCommand>(a->GetId(), before, after), s);
    CHECK(Same(a->Transform(), after));
    u.Execute(std::make_unique<SetParamCommand>(box, BoxObject::kHeight, 99), s);
    CHECK(b->GetObject()->Params().Float(BoxObject::kHeight) == 99);   // both instances change
    u.Execute(std::make_unique<LinkCommand>(b->GetId(), a->GetId()), s);
    CHECK(b->Parent() == a);
    u.Execute(std::make_unique<RenameCommand>(a->GetId(), "Renamed"), s);
    u.Execute(std::make_unique<WireColorCommand>(a->GetId(), 0xFF0000), s);
    CHECK(a->Name() == "Renamed" && a->WireColor() == 0xFF0000);
    // Undo everything, newest first.
    CHECK(u.UndoName() == "Color Change");
    for (int i = 0; i < 5; ++i) CHECK(u.Undo(s));
    CHECK(!u.CanUndo() && a->Name() == "A" && a->WireColor() == 0x808080 && b->Parent() == nullptr);
    CHECK(box->Params().Float(BoxObject::kHeight) == 10 && Same(a->Transform(), before));
    // And redo everything.
    for (int i = 0; i < 5; ++i) CHECK(u.Redo(s));
    CHECK(a->Name() == "Renamed" && b->Parent() == a && box->Params().Float(BoxObject::kHeight) == 99);
}

static void MacroIsOneStep() {
    Scene s;
    UndoStack u;
    auto macro = std::make_unique<MacroCommand>("Create");
    auto create = std::make_unique<CreateNodeCommand>("Box01", std::make_shared<BoxObject>(), 0x808080, PRS{});
    CreateNodeCommand* cp = create.get();
    macro->Add(std::move(create));
    u.Execute(std::move(macro), s);
    const Node::Id id = cp->CreatedId();
    u.Execute(std::make_unique<TransformCommand>(id, PRS{}, At(1, 2, 3)), s);
    CHECK(u.UndoCount() == 2 && u.Undo(s) && u.Undo(s) && s.Nodes().empty());
}

static void StackRules() {
    Scene s;
    UndoStack u(3);                                        // three levels
    Node* n = s.CreateNode("N");
    for (int i = 1; i <= 5; ++i) u.Execute(std::make_unique<TransformCommand>(n->GetId(), At(i - 1, 0, 0), At(i, 0, 0)), s);
    CHECK(u.UndoCount() == 3);                             // the two oldest are forgotten
    CHECK(u.Undo(s) && u.Undo(s) && u.Undo(s) && !u.Undo(s));
    CHECK(n->Transform().position.x == 2);                 // back as far as three levels allow
    // A new command after undoing clears the redo list.
    u.Execute(std::make_unique<TransformCommand>(n->GetId(), At(2, 0, 0), At(9, 0, 0)), s);
    CHECK(!u.CanRedo());
    // Clean state: saved, changed, undone back to the saved state.
    UndoStack c;
    CHECK(c.IsClean());                                    // a new scene is clean
    c.Execute(std::make_unique<RenameCommand>(n->GetId(), "X"), s);
    CHECK(!c.IsClean());
    c.MarkClean();
    c.Execute(std::make_unique<RenameCommand>(n->GetId(), "Y"), s);
    CHECK(!c.IsClean() && c.Undo(s) && c.IsClean() && c.Undo(s) && !c.IsClean());
    CHECK(c.Redo(s) && c.IsClean());
    // Saved state lost to a new branch: never clean again until saved.
    c.Undo(s);
    c.Execute(std::make_unique<RenameCommand>(n->GetId(), "Z"), s);
    CHECK(!c.IsClean() && c.Undo(s) && !c.IsClean());
}

static void SelectionAndDelete() {
    Scene s;
    UndoStack u;
    Node* a = s.CreateNode("A");
    Node* b = s.CreateNode("B");
    Node* c = s.CreateNode("C");
    s.Link(c, b);                                          // C is B's child
    const Node::Id ia = a->GetId(), ib = b->GetId(), ic = c->GetId();
    s.SetSelection({ia, 999, ia});                         // unknown ids and duplicates dropped
    CHECK(s.Selection().size() == 1 && s.IsSelected(ia));
    // Selecting is an undo step.
    u.Execute(std::make_unique<SelectCommand>(std::vector<Node::Id>{ia, ib}), s);
    CHECK(u.UndoName() == "Select" && s.Selection().size() == 2);
    CHECK(u.Undo(s) && s.Selection().size() == 1 && s.IsSelected(ia));
    CHECK(u.Redo(s) && s.IsSelected(ib));
    // Deleting the selection: one step; C (B's child) survives, re-linked to the root.
    u.Execute(std::make_unique<DeleteSelectionCommand>(), s);
    CHECK(u.UndoName() == "Delete" && s.Nodes().size() == 1 && s.FindById(ic) && s.Selection().empty());
    CHECK(s.FindById(ic)->Parent() == nullptr);
    // Undo brings both back, selected, with C under B again.
    CHECK(u.Undo(s) && s.Nodes().size() == 3 && s.IsSelected(ia) && s.IsSelected(ib));
    CHECK(s.FindById(ic)->Parent() == s.FindById(ib));
    // Deleting a node directly drops it from the selection.
    s.DeleteNode(s.FindById(ia));
    CHECK(!s.IsSelected(ia) && s.Selection().size() == 1);
}

int main() {
    SelectionAndDelete();
    CreateUndoRedo();
    DeleteRestoresHierarchyExactly();
    TransformParamLinkRenameColor();
    MacroIsOneStep();
    StackRules();
    if (failures == 0) std::printf("undo: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
