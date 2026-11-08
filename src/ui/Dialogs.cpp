#include "ui/Dialogs.h"
#include <commctrl.h>
#include <commdlg.h>
#include <psapi.h>
#include <cstdio>
#include <vector>

namespace ui {

namespace {

// ---- an in-memory dialog template -------------------------------------------------------

class Template {
public:
    Template(const wchar_t* title, short cx, short cy) {
        DLGTEMPLATE d{};
        d.style = DS_SETFONT | DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;
        d.cx = cx; d.cy = cy;
        Push(&d, sizeof(d));
        t_.push_back(0); t_.push_back(0);
        Str(title);
        t_.push_back(8);
        Str(L"MS Sans Serif");
    }
    void Add(const wchar_t* cls, const wchar_t* text, WORD id, DWORD style, short x, short y, short cx, short cy) {
        if (t_.size() % 2) t_.push_back(0);
        DLGITEMTEMPLATE di{};
        di.style = style | WS_CHILD | WS_VISIBLE;
        di.x = x; di.y = y; di.cx = cx; di.cy = cy; di.id = id;
        Push(&di, sizeof(di));
        Str(cls);
        Str(text);
        t_.push_back(0);
        ++count_;
    }
    const DLGTEMPLATE* Get() {
        reinterpret_cast<DLGTEMPLATE*>(t_.data())->cdit = count_;
        return reinterpret_cast<const DLGTEMPLATE*>(t_.data());
    }

private:
    void Push(const void* p, size_t bytes) { const WORD* w = static_cast<const WORD*>(p); t_.insert(t_.end(), w, w + bytes / 2); }
    void Str(const wchar_t* s) { while (*s) t_.push_back(static_cast<WORD>(*s++)); t_.push_back(0); }
    std::vector<WORD> t_;
    WORD count_ = 0;
};

// ---- Clone Options -------------------------------------------------------------------------

constexpr WORD kCopy = 101, kInstance = 102, kReference = 103, kName = 104;

struct CloneState { CloneChoice* choice; std::wstring suggested; bool single; };

INT_PTR CALLBACK CloneProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = reinterpret_cast<CloneState*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        st = reinterpret_cast<CloneState*>(lp);
        CheckRadioButton(dlg, kCopy, kReference, kCopy);
        EnableWindow(GetDlgItem(dlg, kReference), FALSE);   // maxlike has no references yet
        SetDlgItemTextW(dlg, kName, st->suggested.c_str());
        EnableWindow(GetDlgItem(dlg, kName), st->single);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            st->choice->ok = true;
            st->choice->instance = IsDlgButtonChecked(dlg, kInstance) == BST_CHECKED;
            wchar_t name[256] = L"";
            GetDlgItemTextW(dlg, kName, name, 256);
            st->choice->name = name;
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) { EndDialog(dlg, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}

// ---- Summary Info --------------------------------------------------------------------------

constexpr WORD kList = 201, kDescription = 202, kSaveToFile = 203, kPluginInfo = 204, kClose = 205;

struct SummaryState { const scene::Summary* summary; std::wstring* description; };

std::wstring W(const std::string& s) { return std::wstring(s.begin(), s.end()); }

INT_PTR CALLBACK SummaryProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = reinterpret_cast<SummaryState*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        st = reinterpret_cast<SummaryState*>(lp);
        SetDlgItemTextW(dlg, kDescription, st->description->c_str());
        const HWND list = GetDlgItem(dlg, kList);
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT);
        const wchar_t* cols[] = {L"Name (Type)", L"Verts", L"Faces", L"Cast Shadows", L"Receive Shadows", L"Motion Blur",
                                 L"Hidden", L"Frozen", L"Material (Type)"};
        const int widths[] = {120, 50, 50, 55, 60, 50, 45, 45, 100};
        for (int i = 0; i < 9; ++i) {
            LVCOLUMNW c{};
            c.mask = LVCF_TEXT | LVCF_WIDTH;
            c.pszText = const_cast<wchar_t*>(cols[i]);
            c.cx = widths[i];
            ListView_InsertColumn(list, i, &c);
        }
        int row = 0;
        for (const scene::SummaryRow& r : st->summary->rows) {
            std::wstring name = W(r.name) + L" (" + W(r.type) + L")";
            LVITEMW it{};
            it.mask = LVIF_TEXT;
            it.iItem = row;
            it.pszText = const_cast<wchar_t*>(name.c_str());
            ListView_InsertItem(list, &it);
            // Shadows on, the rest off, and R3's default material (as R3.1 shows a new teapot).
            const std::wstring cells[] = {std::to_wstring(r.verts), std::to_wstring(r.faces), L"Yes", L"Yes", L"No", L"No", L"No", L"default (Std)"};
            for (int c = 0; c < 8; ++c) ListView_SetItemText(list, row, c + 1, const_cast<wchar_t*>(cells[c].c_str()));
            ++row;
        }
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case kSaveToFile: {
            wchar_t path[MAX_PATH] = L"summary.txt";
            OPENFILENAMEW ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = dlg;
            ofn.lpstrFilter = L"Text (*.txt)\0*.txt\0";
            ofn.lpstrFile = path;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrDefExt = L"txt";
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
            if (GetSaveFileNameW(&ofn)) {
                wchar_t desc[4096] = L"";
                GetDlgItemTextW(dlg, kDescription, desc, 4096);
                const std::wstring text = SummaryText(*st->summary, desc);
                HANDLE f = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (f != INVALID_HANDLE_VALUE) {
                    std::string utf8;
                    for (wchar_t ch : text) utf8 += ch < 0x80 ? static_cast<char>(ch) : '?';
                    DWORD written = 0;
                    WriteFile(f, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
                    CloseHandle(f);
                }
            }
            return TRUE;
        }
        case kPluginInfo:
            MessageBoxW(dlg, L"Plug-In Info (not implemented yet)", L"maxlike", MB_OK | MB_ICONINFORMATION);
            return TRUE;
        case kClose: case IDCANCEL: {
            wchar_t desc[4096] = L"";
            GetDlgItemTextW(dlg, kDescription, desc, 4096);
            *st->description = desc;
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        }
        break;
    }
    return FALSE;
}

std::wstring Mb(double bytes) { wchar_t b[32]; std::swprintf(b, 32, L"%.1fM", bytes / (1024.0 * 1024.0)); return b; }

} // namespace

CloneChoice ShowCloneDialog(HWND parent, const std::wstring& suggestedName, bool singleObject) {
    Template t(L"Clone Options", 150, 104);
    t.Add(L"Button", L"Object", 0xFFFF, BS_GROUPBOX, 7, 5, 136, 46);
    t.Add(L"Button", L"Copy", kCopy, BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 14, 16, 60, 10);
    t.Add(L"Button", L"Instance", kInstance, BS_AUTORADIOBUTTON, 14, 27, 60, 10);
    t.Add(L"Button", L"Reference", kReference, BS_AUTORADIOBUTTON, 14, 38, 60, 10);
    t.Add(L"Static", L"Name:", 0xFFFF, SS_LEFT, 7, 59, 24, 9);
    t.Add(L"Edit", L"", kName, ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP | WS_GROUP, 33, 57, 110, 12);
    t.Add(L"Button", L"OK", IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP, 36, 82, 50, 14);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP, 93, 82, 50, 14);
    CloneChoice choice;
    CloneState st{&choice, suggestedName, singleObject};
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), t.Get(), parent, CloneProc, reinterpret_cast<LPARAM>(&st));
    return choice;
}

void ShowSummaryDialog(HWND parent, const scene::Summary& s, std::wstring& description) {
    // Pixel positions from r3.1-summary-teapot.png (dialog client 788 x 480 px), as dialog
    // units for MS Sans Serif 8 (x / 1.5, y / 1.625).
    Template t(L"Summary Info (at Frame 0)", 525, 296);
    auto num = [](size_t n) { return std::to_wstring(n); };
    t.Add(L"Button", L"Scene Totals", 0xFFFF, BS_GROUPBOX, 4, 4, 59, 58);
    const wchar_t* labels[] = {L"Objects:", L"Shapes:", L"Lights:", L"Cameras:", L"Helpers:", L"Space Warps:", L"Total:"};
    const size_t values[] = {s.objects, 0, 0, 0, 0, 0, s.total};
    static std::wstring texts[7];
    for (int i = 0; i < 7; ++i) {
        texts[i] = num(values[i]);
        t.Add(L"Static", labels[i], 0xFFFF, SS_RIGHT, 8, static_cast<short>(13 + 7 * i), 36, 7);
        t.Add(L"Static", texts[i].c_str(), 0xFFFF, SS_LEFT, 47, static_cast<short>(13 + 7 * i), 14, 7);
    }
    static std::wstring verts, faces;
    verts = num(s.verts);
    faces = num(s.faces);
    t.Add(L"Button", L"Mesh Totals", 0xFFFF, BS_GROUPBOX, 67, 4, 48, 28);
    t.Add(L"Static", L"Vertices:", 0xFFFF, SS_RIGHT, 70, 13, 26, 7);
    t.Add(L"Static", verts.c_str(), 0xFFFF, SS_LEFT, 98, 13, 16, 7);
    t.Add(L"Static", L"Faces:", 0xFFFF, SS_RIGHT, 70, 21, 26, 7);
    t.Add(L"Static", faces.c_str(), 0xFFFF, SS_LEFT, 98, 21, 16, 7);
    static std::wstring physical, virt;
    PROCESS_MEMORY_COUNTERS pmc{};
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    physical = L"Physical: " + Mb(static_cast<double>(pmc.WorkingSetSize)) + L" / " + Mb(static_cast<double>(ms.ullTotalPhys));
    virt = L"Virtual: " + Mb(static_cast<double>(pmc.PagefileUsage)) + L" / " + Mb(static_cast<double>(ms.ullTotalVirtual));
    t.Add(L"Button", L"Memory Usage", 0xFFFF, BS_GROUPBOX, 119, 4, 74, 28);
    t.Add(L"Static", physical.c_str(), 0xFFFF, SS_LEFT, 122, 13, 70, 7);
    t.Add(L"Static", virt.c_str(), 0xFFFF, SS_LEFT, 122, 21, 70, 7);
    t.Add(L"Button", L"Rendering", 0xFFFF, BS_GROUPBOX, 197, 4, 91, 34);
    t.Add(L"Static", L"Last Frame Time:  0:00:00", 0xFFFF, SS_RIGHT, 200, 13, 85, 7);
    t.Add(L"Static", L"Last Animation Time:  0:00:00", 0xFFFF, SS_RIGHT, 200, 20, 85, 7);
    t.Add(L"Static", L"Last Video Post Time:  0:00:00", 0xFFFF, SS_RIGHT, 200, 27, 85, 7);
    t.Add(L"Button", L"Description", 0xFFFF, BS_GROUPBOX, 67, 37, 221, 39);
    t.Add(L"Edit", L"", kDescription, ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_BORDER | WS_TABSTOP, 70, 46, 215, 27);
    t.Add(L"Static", L"Summary Info (at Frame 0)", 0xFFFF, SS_LEFT, 4, 80, 120, 8);
    t.Add(L"Button", L"Objects", 0xFFFF, BS_GROUPBOX, 4, 88, 284, 85);
    t.Add(WC_LISTVIEWW, L"", kList, LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER | WS_TABSTOP, 7, 97, 278, 73);
    t.Add(L"Button", L"Save to File...", kSaveToFile, BS_PUSHBUTTON | WS_TABSTOP, 4, 178, 40, 11);
    t.Add(L"Button", L"Plug-In Info...", kPluginInfo, BS_PUSHBUTTON | WS_TABSTOP, 62, 178, 40, 11);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_DISABLED, 222, 178, 30, 11);
    t.Add(L"Button", L"Close", kClose, BS_DEFPUSHBUTTON | WS_TABSTOP, 256, 178, 32, 11);
    SummaryState st{&s, &description};
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), t.Get(), parent, SummaryProc, reinterpret_cast<LPARAM>(&st));
}

std::wstring SummaryText(const scene::Summary& s, const std::wstring& description) {
    std::wstring t = L"Summary Info (at Frame 0)\r\n\r\nScene Totals\r\n";
    t += L"  Objects: " + std::to_wstring(s.objects) + L"\r\n  Shapes: 0\r\n  Lights: 0\r\n  Cameras: 0\r\n  Helpers: 0\r\n  Space Warps: 0\r\n";
    t += L"  Total: " + std::to_wstring(s.total) + L"\r\n\r\nMesh Totals\r\n";
    t += L"  Vertices: " + std::to_wstring(s.verts) + L"\r\n  Faces: " + std::to_wstring(s.faces) + L"\r\n\r\nDescription\r\n  " + description + L"\r\n\r\nObjects\r\n";
    for (const scene::SummaryRow& r : s.rows)
        t += L"  " + W(r.name) + L" (" + W(r.type) + L")\t" + std::to_wstring(r.verts) + L"\t" + std::to_wstring(r.faces) + L"\r\n";
    return t;
}

} // namespace ui
