// Groups (Phase 10): selection through groups, and Group / Ungroup / Explode / Detach /
// Attach / Open with undo, checked on positions in the world.
#include "scene/Groups.h"
#include "scene/Primitives.h"
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
static bool Near(const math::Vec3& a, const math::Vec3& b) { return math::Length(a - b) < 1e-9; }
static math::Vec3 Pivot(const scene::Node* n) { return math::TransformPoint(n->WorldMatrix(), {0, 0, 0}); }
using namespace scene::groups;

int main() {
    scene::Scene s;
    scene::UndoStack undo;
    auto box = [&](const char* name, math::Vec3 at) {
        scene::Node* n = s.CreateNode(name);
        n->SetObject(std::make_shared<scene::BoxObject>(10, 10, 10));
        n->Transform().position = at;
        return n;
    };
    scene::Node* a = box("Box01", {0, 0, 0});
    scene::Node* b = box("Box02", {30, 0, 0});
    scene::Node* c = box("Box03", {100, 0, 0});
    const scene::Node::Id ia = a->GetId(), ib = b->GetId(), ic = c->GetId();

    // Group Box01 and Box02: a closed head with both, nothing moves, the head selected and
    // at their centre (x 15, half the boxes' height up: z 5).
    auto g = MakeGroup(s, {ia, ib}, "Group01");
    undo.Execute(std::move(g), s);
    scene::Node* head = s.Find("Group01");
    CHECK(head && head->IsGroupHead() && !head->GroupOpen() && !head->GetObject());
    CHECK(head && head->Children().size() == 2 && s.FindById(ia)->Parent() == head);
    CHECK(Near(Pivot(s.FindById(ia)), {0, 0, 0}) && Near(Pivot(s.FindById(ib)), {30, 0, 0}));
    CHECK(head && Near(Pivot(head), {15, 0, 5}));
    CHECK(s.Selection().size() == 1 && head && s.Selection()[0] == head->GetId());
    // Picking through the group.
    CHECK(SelectableFor(s.FindById(ia)) == head && SelectableFor(s.FindById(ic)) == s.FindById(ic));
    CHECK(Selectable(s, {ia, ib, ic}).size() == 2);
    CHECK(ShownSelected(s, *s.FindById(ia)) && !ShownSelected(s, *s.FindById(ic)));
    CHECK(AllSelectable(s).size() == 2);                                   // the group and Box03
    CHECK(WithMembers(s, {head->GetId()}).size() == 3);
    // Moving the head moves the group.
    head->Transform().position = head->Transform().position + math::Vec3{0, 50, 0};
    CHECK(Near(Pivot(s.FindById(ib)), {30, 50, 0}));
    head->Transform().position = head->Transform().position - math::Vec3{0, 50, 0};
    // Open: members one by one; Close again. Undoable.
    undo.Execute(std::make_unique<scene::GroupOpenCommand>(head->GetId(), true), s);
    CHECK(SelectableFor(s.FindById(ia)) == s.FindById(ia) && AllSelectable(s).size() == 4);
    CHECK(undo.Undo(s) && SelectableFor(s.FindById(ia)) == head);
    // Undo the group: members back at the top, in place; the head gone. Redo: back, closed.
    CHECK(undo.Undo(s) && !s.Find("Group01") && !s.FindById(ia)->Parent());
    CHECK(Near(Pivot(s.FindById(ia)), {0, 0, 0}) && Near(Pivot(s.FindById(ib)), {30, 0, 0}));
    CHECK(undo.Redo(s) && (head = s.Find("Group01")) && head->IsGroupHead() && head->Children().size() == 2);
    // Nest: group Group01 with Box03 into Group02, then Explode: no groups left, all in place.
    undo.Execute(MakeGroup(s, {head->GetId(), ic}, "Group02"), s);
    scene::Node* outer = s.Find("Group02");
    CHECK(outer && SelectableFor(s.FindById(ia)) == outer);               // the outermost closed group
    undo.Execute(MakeUngroup(s, {outer->GetId()}, true), s);
    CHECK(!s.Find("Group01") && !s.Find("Group02") && s.Nodes().size() == 3);
    for (scene::Node::Id id : {ia, ib, ic}) CHECK(!s.FindById(id)->Parent());
    CHECK(Near(Pivot(s.FindById(ib)), {30, 0, 0}) && Near(Pivot(s.FindById(ic)), {100, 0, 0}));
    CHECK(s.Selection().size() == 3);
    CHECK(undo.Undo(s) && s.Find("Group02") && s.Find("Group01") && s.Find("Group01")->Parent() == s.Find("Group02"));
    // Undoing restores the selection too: the group, selected (it was before Explode).
    CHECK(s.Selection().size() == 1 && s.Selection()[0] == s.Find("Group02")->GetId());
    // Ungroup (not Explode) of the outer: Group01 and Box03 freed, Group01 still a group.
    undo.Execute(MakeUngroup(s, {s.Find("Group02")->GetId()}, false), s);
    CHECK(!s.Find("Group02") && s.Find("Group01") && !s.Find("Group01")->Parent() && s.Find("Group01")->IsGroupHead());
    // Detach Box02 from Group01 (opened), then Attach Box03 to it.
    head = s.Find("Group01");
    undo.Execute(std::make_unique<scene::GroupOpenCommand>(head->GetId(), true), s);
    undo.Execute(MakeDetach(s, {ib}), s);
    CHECK(!s.FindById(ib)->Parent() && head->Children().size() == 1 && Near(Pivot(s.FindById(ib)), {30, 0, 0}));
    undo.Execute(MakeAttach(s, {ic}, head->GetId()), s);
    CHECK(s.FindById(ic)->Parent() == head && Near(Pivot(s.FindById(ic)), {100, 0, 0}));
    CHECK(MakeAttach(s, {head->GetId()}, head->GetId()) == nullptr);       // not into itself
    CHECK(MakeDetach(s, {ic}) != nullptr && MakeDetach(s, {ib}) == nullptr);
    if (failures == 0) std::printf("groups: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
