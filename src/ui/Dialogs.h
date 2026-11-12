#pragma once
#include <windows.h>          // first: the scene headers must see its GetObject macro (as everywhere else)
#include "scene/Summary.h"
#include <string>
#include <vector>
#include <functional>
#include "math/Mat4.h"

// Phase 9.3 dialogs, built from in-memory templates (as the More... dialog).
namespace ui {

// Edit > Clone's "Clone Options" (NOTE: from memory, no R3.1 screenshot): Object Copy /
// Instance / Reference (Reference not available), and the clone's name (one object only).
struct CloneChoice { bool ok = false; bool instance = false; std::wstring name; };
CloneChoice ShowCloneDialog(HWND parent, const std::wstring& suggestedName, bool singleObject);

// File > Summary Info, laid out after r3.1-summary-teapot.png: Scene Totals, Mesh Totals,
// Memory Usage, Rendering, Description, the objects table, Save to File..., Plug-In Info...,
// Cancel (greyed), Close. NOTE: layout converted from pixels, not pixel-checked.
void ShowSummaryDialog(HWND parent, const scene::Summary& summary, std::wstring& description);

// Edit Mesh > Detach (NOTE: from memory, no R3.1 screenshot): "Detach as:" and the name,
// Detach To Element (not available), Detach As Clone.
struct DetachChoice { bool ok = false; bool asClone = false; std::wstring name; };
DetachChoice ShowDetachDialog(HWND parent, const std::wstring& suggestedName);

// Tools > Transform Type-In (F12), a modeless floater (NOTE: layout from memory, no
// R3.1 screenshot): "Absolute:World" and "Offset:World" X / Y / Z fields. Enter applies the
// group being typed in. MainFrame keeps its title and values current.
class TypeInDialog {
public:
    std::function<void(bool absolute, const math::Vec3& value)> onApply;
    void Show(HWND owner);
    bool IsOpen() const { return hwnd_ && IsWindow(hwnd_); }
    // The tool's name ("Move"), whether anything can be typed, whether Absolute applies (one
    // object), its values, and the Offset fields' neutral value (0, Scale 100).
    void Update(const std::wstring& tool, bool enabled, bool absolute, const math::Vec3& value, double neutral);

private:
    static INT_PTR CALLBACK Proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp);
    HWND hwnd_ = nullptr;
    double neutral_ = 0;
};

// Tools > Mirror and Tools > Array (9.10; NOTE: layouts from memory, no R3.1 screenshot;
// R3's live preview while the dialogs are open is not built).
struct MirrorChoice { bool ok = false; int axis = 0; double offset = 0; int clone = 0; };   // axis: X Y Z XY YZ ZX; clone: 0 no, 1 copy, 2 instance
MirrorChoice ShowMirrorDialog(HWND parent, const std::wstring& system);
struct ArrayChoice {
    bool ok = false;
    math::Vec3 move{0, 0, 0}, rotate{0, 0, 0}, scale{100, 100, 100};
    bool instance = false;
    int dimensions = 1, count1 = 10, count2 = 1, count3 = 1;
    math::Vec3 offset2{0, 0, 0}, offset3{0, 0, 0};
};
ArrayChoice ShowArrayDialog(HWND parent);

// Tools > Align's "Align Selection (target)" (10.2; NOTE: layout from memory; R3's live
// preview and Apply button are not built).
struct AlignChoice {
    bool ok = false;
    bool position[3] = {false, false, false};
    int current = 1, target = 1;                   // 0 Minimum, 1 Center, 2 Pivot Point, 3 Maximum
    bool orientation[3] = {false, false, false};
    bool scale[3] = {false, false, false};
};
AlignChoice ShowAlignDialog(HWND parent, const std::wstring& targetName);

// A name asked for in a small dialog (Group > Group: "Group name:"; NOTE: from memory).
// Empty if cancelled.
std::wstring AskName(HWND parent, const std::wstring& title, const std::wstring& label, const std::wstring& suggested);

// Phase 11.2, laid out from R3.1's screenshots (r3.1-dlg-new.png, r3.1-dlg-merge.png).
// File > New's "New Scene": 0 cancelled, 1 Keep Objects and Hierarchy, 2 Keep Objects,
// 3 New All.
int ShowNewSceneDialog(HWND parent);
// File > Merge's object list: the names (with their depth below the top, whether each is a
// group, and its parent's index); returns the chosen indices (empty: cancelled).
struct MergeItem { std::wstring name; int depth = 0; bool group = false; int parent = -1; };
std::vector<int> ShowMergeDialog(HWND parent, const std::wstring& fileName, const std::vector<MergeItem>& items);

// The Summary as text (Save to File...).
std::wstring SummaryText(const scene::Summary& summary, const std::wstring& description);

} // namespace ui
