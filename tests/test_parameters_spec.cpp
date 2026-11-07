// Unit tests for the Parameters / Creation Method layout, against the R3.1 screenshot
// of a Box being created (Parameters title at row 376, frame ending at row 559).
#include "ui/ParametersSpec.h"
#include "scene/Modifier.h"
#include <memory>
#include "scene/CreationTool.h"
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

using namespace ui::params;

int main() {
    const Spec box = ParametersFor(0);
    // Measured rows (title top 376): spinners at 400, 421, 442, 470, 491, 512; the
    // checkbox at 538; the frame ends at 559 (content 168 below the title's bottom).
    const int tops[] = {400, 421, 442, 470, 491, 512, 538};
    CHECK(box.rows.size() == 7);
    for (size_t i = 0; i < box.rows.size() && i < 7; ++i) CHECK(376 + box.rows[i].top == tops[i]);
    CHECK(box.contentHeight == 168);
    CHECK(box.rows[0].label == L"Length:" && box.rows[3].kind == Kind::Int && box.rows[6].kind == Kind::Check);
    CHECK(box.rows[2].min < 0);                                   // a box can be built downwards
    CHECK(CreationMethodsFor(0).first == L"Cube" && CreationMethodsFor(0).selected == 1);
    // Every primitive has rows that refer to parameters its object defines.
    for (int type = 0; type < 10; ++type) {
        const Spec s = ParametersFor(type);
        CHECK(!s.rows.empty() && s.contentHeight > 0);
        auto object = scene::MakePrimitive(type);
        for (const Row& r : s.rows)
            if (r.paramIndex >= 0) CHECK(object->Params().Has(static_cast<size_t>(r.paramIndex)));
    }
    // Bend's rollout, measured (r3.1-modify-bend.png; title top 378): groups at 403..462,
    // 475..513, 527..605; Angle and Direction spinners at 414, 435 (field x 81); radios at
    // 491 (x 40, 81, 120); Limit Effect at 538 (x 87); limits at 558, 579 (field x 87);
    // the frame's shadow line at 615 (its bottom, as the rollup counts it, at 616); one decimal.
    {
        const Spec b = ModifierParametersFor("Bend");
        CHECK(b.groups.size() == 3 && b.rows.size() == 6);
        CHECK(378 + b.groups[0].top == 403 && 378 + b.groups[0].bottom == 462 && 378 + b.groups[2].bottom == 605);
        CHECK(378 + b.rows[0].top == 414 && b.rows[0].fieldLeft == 81 && b.rows[0].FieldRight() == 134 && b.rows[0].ArrowsRight() == 148);
        CHECK(b.rows[2].kind == Kind::Radio && 378 + b.rows[2].top == 491 && b.rows[2].optionLefts[2] == 120);
        CHECK(b.rows[3].kind == Kind::Check && b.rows[3].checkLeft == 87 && 378 + b.rows[3].top == 538);
        CHECK(378 + b.rows[5].top == 579 && b.rows[5].fieldLeft == 87 && b.rows[5].ArrowsLeft() == 143);
        CHECK(378 + 15 + b.contentHeight == 616 && b.rows[0].decimals == 1);
        CHECK(b.rows[0].paramIndex == scene::BendModifier::kAngle && b.rows[2].paramIndex == scene::BendModifier::kAxis);
        // Every modifier row refers to a parameter the modifier defines.
        auto bend = std::make_shared<scene::BendModifier>(); auto twist = std::make_shared<scene::TwistModifier>();
        auto taper = std::make_shared<scene::TaperModifier>();
        const std::pair<const char*, scene::Modifier*> mods[3] = {{"Bend", bend.get()}, {"Twist", twist.get()}, {"Taper", taper.get()}};
        for (const auto& [name, m] : mods)
            for (const Row& r : ModifierParametersFor(name).rows) CHECK(m->Params().Has(static_cast<size_t>(r.paramIndex)));
        CHECK(ModifierParametersFor("Noise").rows.empty());
    }
    // The Create panel's rows keep the original geometry (field x 90, three decimals).
    CHECK(box.rows[0].fieldLeft == 90 && box.rows[0].FieldRight() == 143 && box.rows[0].ArrowsLeft() == 146 && box.rows[0].decimals == 3);
    // Edit Mesh's rollouts, measured (r3.1-modify-editmesh-vertex.png): Selection (content
    // 195), Soft Selection collapsed, Edit Geometry; Planar Thresh's field x 99..143 with
    // arrows at 146; Hide x 15..86; the second unlabelled group's shadow at title + 233;
    // the Weld group's frame one column wider (x 14..166).
    {
        const auto r = ModifierRollouts("Edit Mesh");
        CHECK(r.size() == 4 && r[0].title == L"Selection" && r[1].title == L"Soft Selection" && !r[1].startsOpen);
        CHECK(r[3].title == L"Surface Properties" && r[2].contentHeight == 503);   // measured from the scrolled screenshot
        CHECK(r[0].contentHeight == 195 && r[2].title == L"Edit Geometry");
        const Row& planar = r[0].rows[4];
        CHECK(planar.fieldLeft == 99 && planar.FieldRight() == 143 && planar.ArrowsLeft() == 146 && !planar.enabled);
        CHECK(r[0].rows[5].kind == Kind::Button && r[0].rows[5].left == 15 && r[0].rows[5].right == 86);
        CHECK(r[2].groups[1].bottom == 233 && r[2].groups[2].right == 166 && r[2].groups[2].label == L"Weld");
        CHECK(ModifierRollouts("Bend").size() == 1 && ModifierRollouts("Noise").empty());
        auto em = std::make_shared<scene::EditMeshModifier>(8, std::vector<scene::EditMeshModifier::Move>{});
        for (const auto& spec : r)
            for (const Row& row : spec.rows)
                if (row.paramIndex >= 0) CHECK(em->Params().Has(static_cast<size_t>(row.paramIndex)));
    }
    CHECK(ParametersFor(42).rows.empty());
    // Keyboard Entry: X, Y, Z, then the sizes, then the Create button below them.
    const KeyboardSpec kb = KeyboardEntryFor(0);
    CHECK(kb.fields.rows.size() == 6 && kb.fields.rows[0].paramIndex == kPosX && kb.fields.rows[2].paramIndex == kPosZ);
    CHECK(kb.fields.rows[3].label == L"Length:" && kb.fields.rows[5].label == L"Height:");
    CHECK(kb.fields.rows[3].top == 94 && kb.buttonTop > kb.fields.rows.back().top + 16);
    CHECK(KeyboardEntryFor(2).fields.rows.size() == 4);              // Sphere: X, Y, Z, Radius
    CHECK(KeyboardEntryFor(6).fields.rows.size() == 5);              // Torus: no angles
    CHECK(KeyboardEntryFor(42).fields.rows.empty());
    if (failures == 0) std::printf("parameters spec: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
