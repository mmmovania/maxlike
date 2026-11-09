#pragma once
#include "scene/Summary.h"
#include <windows.h>
#include <string>

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

// The Summary as text (Save to File...).
std::wstring SummaryText(const scene::Summary& summary, const std::wstring& description);

} // namespace ui
