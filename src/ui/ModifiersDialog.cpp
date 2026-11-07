#include "ui/ModifiersDialog.h"
#include <cwctype>

namespace ui {

const std::vector<std::wstring>& ModifierListEntries() {
    static const std::vector<std::wstring> kEntries = {
        L"WORLD-SPACE MODIFIERS", L"  * Camera Map", L"  * Displace Mesh", L"  * MapScaler", L"  * PatchDeform",
        L"  * PathDeform", L"  * Surface Mapper", L"  * SurfDeform",
        L"OBJECT-SPACE MODIFIERS", L"    Affect Region", L"    Camera Map", L"    Cap Holes", L"    DeleteMesh",
        L"    Disp Approx", L"    Displace", L"    Edit Mesh", L"    Face Extrude", L"    FFD 2x2x2", L"    FFD 3x3x3",
        L"    FFD 4x4x4", L"    FFD(box)", L"    FFD(cyl)", L"    Flex", L"    Lattice", L"    Linked XForm",
        L"    Material", L"    MaterialByElement", L"    Melt", L"    MeshSmooth", L"    Mirror", L"    Morpher",
        L"    Normal", L"    Optimize", L"    PatchDeform", L"    PathDeform", L"    Preserve", L"    Push",
        L"    Relax", L"    Ripple", L"    Skew", L"    Skin", L"    Slice", L"    Smooth", L"    Spherify",
        L"    Squeeze", L"    STL-Check",
        // Below the screenshot's visible part, from memory:
        L"    Stretch", L"    Surface Mapper", L"    Tessellate", L"    Vol. Select", L"    Wave", L"    XForm"};
    return kEntries;
}

std::wstring ModifierNameFromEntry(const std::wstring& entry) {
    if (entry.empty() || entry[0] != L' ') return L"";            // a heading
    size_t i = 0;
    while (i < entry.size() && (entry[i] == L' ' || entry[i] == L'*')) ++i;
    return entry.substr(i);
}

namespace {

constexpr int kListId = 100;

// A dialog template built in memory: the list and OK / Cancel (sizes in dialog units,
// converted from the screenshot's pixels for MS Sans Serif 8; NOTE: approximate).
std::vector<WORD> BuildTemplate() {
    std::vector<WORD> t;
    auto push = [&](const void* p, size_t bytes) {
        const WORD* w = static_cast<const WORD*>(p);
        t.insert(t.end(), w, w + bytes / 2);
    };
    auto str = [&](const wchar_t* s) { while (*s) t.push_back(static_cast<WORD>(*s++)); t.push_back(0); };
    auto align = [&] { if (t.size() % 2) t.push_back(0); };
    DLGTEMPLATE d{};
    d.style = DS_SETFONT | DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    d.cdit = 3;
    d.cx = 201;
    d.cy = 380;
    push(&d, sizeof(d));
    t.push_back(0);                 // no menu
    t.push_back(0);                 // default class
    str(L"Modifiers");
    t.push_back(8);                 // point size
    str(L"MS Sans Serif");
    struct Item { DWORD style; short x, y, cx, cy; WORD id; WORD cls; const wchar_t* text; };
    const Item items[3] = {
        {WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_TABSTOP | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, 3, 5, 132, 368, kListId, 0x0083, L""},
        {WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 143, 6, 53, 15, IDOK, 0x0080, L"OK"},
        {WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 143, 23, 53, 15, IDCANCEL, 0x0080, L"Cancel"}};
    for (const Item& it : items) {
        align();
        DLGITEMTEMPLATE di{};
        di.style = it.style;
        di.x = it.x; di.y = it.y; di.cx = it.cx; di.cy = it.cy;
        di.id = it.id;
        push(&di, sizeof(di));
        t.push_back(0xFFFF);
        t.push_back(it.cls);
        str(it.text);
        t.push_back(0);             // no creation data
    }
    return t;
}

struct State { std::wstring chosen; };

void Accept(HWND dlg, State* st) {
    const HWND list = GetDlgItem(dlg, kListId);
    const LRESULT i = SendMessageW(list, LB_GETCURSEL, 0, 0);
    if (i < 0) return;
    const auto& entries = ModifierListEntries();
    if (static_cast<size_t>(i) >= entries.size()) return;
    const std::wstring name = ModifierNameFromEntry(entries[static_cast<size_t>(i)]);
    if (name.empty()) return;       // a heading: nothing to add
    st->chosen = name;
    EndDialog(dlg, IDOK);
}

INT_PTR CALLBACK Proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = reinterpret_cast<State*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        const HWND list = GetDlgItem(dlg, kListId);
        for (const std::wstring& e : ModifierListEntries())
            SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(e.c_str()));
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK || (LOWORD(wp) == kListId && HIWORD(wp) == LBN_DBLCLK)) { Accept(dlg, st); return TRUE; }
        if (LOWORD(wp) == IDCANCEL) { EndDialog(dlg, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}

} // namespace

std::wstring ShowModifiersDialog(HWND parent) {
    const std::vector<WORD> t = BuildTemplate();
    State st;
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), reinterpret_cast<const DLGTEMPLATE*>(t.data()), parent, Proc,
                            reinterpret_cast<LPARAM>(&st));
    return st.chosen;
}

} // namespace ui
