#pragma once
#include <windows.h>          // first: the scene headers must see its GetObject macro (as everywhere else)
#include "scene/Summary.h"
#include <string>
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

// The Summary as text (Save to File...).
std::wstring SummaryText(const scene::Summary& summary, const std::wstring& description);

} // namespace ui
