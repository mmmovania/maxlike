// Unit tests for the Parameters / Creation Method layout, against the R3.1 screenshot
// of a Box being created (Parameters title at row 376, frame ending at row 559).
#include "ui/ParametersSpec.h"
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
    CHECK(ParametersFor(42).rows.empty());
    if (failures == 0) std::printf("parameters spec: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
