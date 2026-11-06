#pragma once
#include <cstddef>
#include <string>
#include <vector>

// What the Create panel's Creation Method and Parameters rollouts show for each
// primitive, and where. No Windows dependency (unit-tested in tests/test_parameters_spec.cpp).
//
// Measured for Box (R3.1 screenshot docs/reference/screenshots/r3.1-box-created.png):
// spinners at title + 24, + 45, + 66, a 7-row gap, + 94, + 115, + 136, the checkbox at
// + 162; the frame ends 21 rows below the last row's top. The other primitives'
// lists are from memory of R3 and limited to parameters maxlike implements
// (NOTE: unverified), laid out by the same rule.
namespace ui::params {

enum class Kind { Float, Int, Check };

struct Row {
    std::wstring label;   // "Length:" (spinners) or "Generate Mapping Coords." (checks)
    int paramIndex;       // Max parameter index; -1 = shown but not stored yet
    Kind kind;
    double min, max, step;
    int top;              // rows below the rollout's title top
};

struct Spec {
    std::vector<Row> rows;
    int contentHeight;    // title bottom -> frame bottom
};

// type: Object Type order (Box, Cone, Sphere, GeoSphere, Cylinder, Tube, Torus, Pyramid,
// Teapot, Plane).
Spec ParametersFor(int type);

// Measured geometry of a Parameters row.
inline constexpr int kLabelRight = 87;                    // labels right-aligned, ending here
inline constexpr int kFieldLeft = 90, kFieldRight = 143;  // the number field
inline constexpr int kArrowsLeft = 146, kArrowsRight = 157;
inline constexpr int kSpinnerHeight = 16;
inline constexpr int kCheckLeft = 18, kCheckTextLeft = 37;
inline constexpr int kFloatDecimals = 3;                  // "62.572"

// Creation Method: two radio buttons (measured for Box: "Cube" at x 34 and "Box" at
// x 102, both at title + 22; text 4 px after the circle). Labels for other primitives
// from memory (NOTE: unverified).
struct Methods {
    std::wstring first, second;
    int selected;          // the default
};
Methods CreationMethodsFor(int type);
inline constexpr int kMethodRadioLeft[2] = {34, 102};
inline constexpr int kMethodTop = 22, kRadioSize = 12;
inline constexpr int kCreationMethodContent = 28;

// Keyboard Entry: X, Y, Z (world position of the pivot), a gap, the primitive's sizes
// (its Parameters' size rows), then a Create button. Laid out by the Parameters rule;
// NOTE: R3's exact Keyboard Entry layout and default values are unverified.
inline constexpr int kPosX = -10, kPosY = -11, kPosZ = -12;   // paramIndex of X, Y, Z
struct KeyboardSpec {
    Spec fields;               // spinner rows (X, Y, Z, then sizes)
    int buttonTop;             // the Create button's top, rows below the title's top
};
KeyboardSpec KeyboardEntryFor(int type);
inline constexpr int kCreateButtonLeft = 57, kCreateButtonWidth = 69, kCreateButtonHeight = 18;

} // namespace ui::params
