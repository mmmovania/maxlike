#pragma once
#include <cstddef>
#include <string>
#include <utility>
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

enum class Kind { Float, Int, Check, Radio,
                  Button,     // a push button (left..right, height); clicking reports its label
                  Label,      // static text from `left` (or right-aligned to `right`); "{selection}" is replaced
                  Levels };   // the five sub-object level icons (Edit Mesh)

// Measured geometry of a Parameters row (Create panel); rows may override the field,
// checkbox and decimals (Bend's rollout: fields at x 81 and 87, one decimal).
inline constexpr int kLabelRight = 87;                    // labels right-aligned, ending here
inline constexpr int kFieldLeft = 90, kFieldRight = 143;  // the number field
inline constexpr int kArrowsLeft = 146, kArrowsRight = 157;
inline constexpr int kSpinnerHeight = 16;
inline constexpr int kCheckLeft = 18, kCheckTextLeft = 37;
inline constexpr int kFloatDecimals = 3;                  // "62.572"

struct Row {
    Row() = default;
    Row(std::wstring label_, int paramIndex_, Kind kind_, double min_, double max_, double step_, int top_)
        : label(std::move(label_)), paramIndex(paramIndex_), kind(kind_), min(min_), max(max_), step(step_), top(top_) {}

    std::wstring label;   // "Length:" (spinners) or "Generate Mapping Coords." (checks)
    int paramIndex = -1;  // Max parameter index; -1 = shown but not stored yet
    Kind kind = Kind::Float;
    double min = 0, max = 0, step = 1;
    int top = 0;          // rows below the rollout's title top
    int fieldLeft = kFieldLeft;       // spinners: the number field's left; label ends 3 px before it
    int fieldWidth = 54;              // spinners: the field's width (measured: 54, 45 or 56)
    int arrowsLeft = -1;              // spinners: the arrows' left (-1: 3 px after the field)
    bool enabled = true;              // drawn and handled as disabled when false
    int left = 0, right = 0, height = 18;   // buttons and labels
    bool alignRight = false;          // labels
    int decimals = kFloatDecimals;    // floats shown with this many decimals
    int checkLeft = kCheckLeft;       // checks: the box's left; the text starts 19 px after it
    std::vector<std::wstring> options;   // radios: labels, value = the option's index
    std::vector<int> optionLefts;        // radios: each circle's left (label 17 px after it)

    int FieldRight() const { return fieldLeft + fieldWidth - 1; }
    int ArrowsLeft() const { return arrowsLeft >= 0 ? arrowsLeft : FieldRight() + 3; }
    int ArrowsRight() const { return ArrowsLeft() + 11; }
    int LabelRight() const { return fieldLeft - 3; }
};

// An etched group box with a label breaking its top line (x 17..163; measured on Bend).
struct Group {
    int top, bottom;      // rows below the title top (the shadow line)
    std::wstring label;   // empty: no label and no gap
    int gapEnd;           // the top line is cleared from gapStart to here, around the label
    int left = 17, right = 163;           // the frame (Bend: 17..163; Edit Geometry: 14..165)
    int gapStart = 24, labelLeft = 27;    // Weld: 21 and 23
};

struct Spec {
    std::vector<Row> rows;
    int contentHeight = 0;    // title bottom -> frame bottom
    std::vector<Group> groups;
    std::wstring title = L"Parameters";
    bool startsOpen = true;
};

// All rollouts a modifier shows below the Modifier Stack, in order (Bend: its
// Parameters; Edit Mesh: Selection, Soft Selection, Edit Geometry).
// `level`: Edit Mesh's sub-object level (0 Vertex .. 4 Element; 0 also with Sub-Object
// off), which decides the Edit Geometry buttons that are available (Phase 10.7).
// `sliceMode`: Slice Plane is on, which makes Slice available (10.9).
std::vector<Spec> ModifierRollouts(const std::string& className, int level = 0, bool sliceMode = false);

// A modifier's Parameters rollout, by class name ("Bend", "Taper", "Twist"); empty for
// others. Bend's is measured (r3.1-modify-bend.png); Taper's and Twist's follow the same
// layout (NOTE: unverified).
Spec ModifierParametersFor(const std::string& className);

// type: Object Type order (Box, Cone, Sphere, GeoSphere, Cylinder, Tube, Torus, Pyramid,
// Teapot, Plane).
Spec ParametersFor(int type);

// Measured geometry of a Parameters row.

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
