#include "ui/Dialogs.h"
#include <cwctype>
#include "win/App.h"
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

// ---- Detach -------------------------------------------------------------------------------

constexpr WORD kDetachName = 301, kToElement = 302, kAsClone = 303;
struct DetachState { DetachChoice* choice; std::wstring suggested; };

INT_PTR CALLBACK DetachProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = reinterpret_cast<DetachState*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        st = reinterpret_cast<DetachState*>(lp);
        SetDlgItemTextW(dlg, kDetachName, st->suggested.c_str());
        EnableWindow(GetDlgItem(dlg, kToElement), FALSE);   // maxlike keeps one object per mesh
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            wchar_t name[256] = L"";
            GetDlgItemTextW(dlg, kDetachName, name, 256);
            st->choice->ok = true;
            st->choice->name = name;
            st->choice->asClone = IsDlgButtonChecked(dlg, kAsClone) == BST_CHECKED;
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) { EndDialog(dlg, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}

} // namespace

// ---- Transform Type-In ------------------------------------------------------------------

namespace {
constexpr WORD kAbsX = 401, kAbsY = 402, kAbsZ = 403, kOffX = 404, kOffY = 405, kOffZ = 406;
std::wstring Num(double v) { wchar_t b[32]; std::swprintf(b, 32, L"%.3f", v); return b; }
double Field(HWND dlg, WORD id) { wchar_t b[64] = L""; GetDlgItemTextW(dlg, id, b, 64); return _wtof(b); }
} // namespace

INT_PTR CALLBACK TypeInDialog::Proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* self = reinterpret_cast<TypeInDialog*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK && self) {
            // Enter: the group whose field has the focus.
            const int focus = GetDlgCtrlID(GetFocus());
            const bool absolute = focus >= kAbsX && focus <= kAbsZ;
            const WORD first = absolute ? kAbsX : kOffX;
            const math::Vec3 v{Field(dlg, first), Field(dlg, static_cast<WORD>(first + 1)), Field(dlg, static_cast<WORD>(first + 2))};
            if (self->onApply) self->onApply(absolute, v);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) { DestroyWindow(dlg); return TRUE; }
        break;
    case WM_CLOSE:
        DestroyWindow(dlg);
        return TRUE;
    case WM_DESTROY:
        if (self) self->hwnd_ = nullptr;
        win::App::SetModelessDialog(nullptr);
        break;
    }
    return FALSE;
}

void TypeInDialog::Show(HWND owner) {
    if (IsOpen()) { SetForegroundWindow(hwnd_); return; }
    Template t(L"Move Transform Type-In", 200, 62);
    t.Add(L"Button", L"Absolute:World", 0xFFFF, BS_GROUPBOX, 4, 3, 94, 54);
    t.Add(L"Button", L"Offset:World", 0xFFFF, BS_GROUPBOX, 102, 3, 94, 54);
    const wchar_t* axes[3] = {L"X:", L"Y:", L"Z:"};
    for (int i = 0; i < 3; ++i) {
        const short y = static_cast<short>(14 + 13 * i);
        t.Add(L"Static", axes[i], 0xFFFF, SS_RIGHT, 8, static_cast<short>(y + 2), 12, 8);
        t.Add(L"Edit", L"", static_cast<WORD>(kAbsX + i), ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 23, y, 70, 11);
        t.Add(L"Static", axes[i], 0xFFFF, SS_RIGHT, 106, static_cast<short>(y + 2), 12, 8);
        t.Add(L"Edit", L"", static_cast<WORD>(kOffX + i), ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 121, y, 70, 11);
    }
    // Modeless: no OK button; Enter arrives as IDOK through IsDialogMessage.
    hwnd_ = CreateDialogIndirectParamW(GetModuleHandleW(nullptr), t.Get(), owner, Proc, reinterpret_cast<LPARAM>(this));
    win::App::SetModelessDialog(hwnd_);
    ShowWindow(hwnd_, SW_SHOW);
}

void TypeInDialog::Update(const std::wstring& tool, bool enabled, bool absolute, const math::Vec3& value, double neutral) {
    if (!IsOpen()) return;
    SetWindowTextW(hwnd_, (tool + L" Transform Type-In").c_str());
    neutral_ = neutral;
    const double abs[3] = {value.x, value.y, value.z};
    for (int i = 0; i < 3; ++i) {
        const HWND a = GetDlgItem(hwnd_, kAbsX + i), o = GetDlgItem(hwnd_, kOffX + i);
        EnableWindow(a, enabled && absolute);
        EnableWindow(o, enabled);
        if (GetFocus() != a) SetWindowTextW(a, enabled && absolute ? Num(abs[i]).c_str() : L"");
        if (GetFocus() != o) SetWindowTextW(o, enabled ? Num(neutral).c_str() : L"");
    }
}

// ---- Mirror and Array ---------------------------------------------------------------------

namespace {
constexpr WORD kMirAxis = 501, kMirOffset = 507, kMirClone = 508;          // 501..506 axes, 508..511 clone options
constexpr WORD kArrMove = 601, kArrRot = 604, kArrScale = 607, kArrCopy = 610, kArrInstance = 611, kArrRef = 612;
constexpr WORD kArr1D = 613, kArr2D = 614, kArr3D = 615, kArrCount1 = 616, kArrCount2 = 617, kArrCount3 = 618, kArrOff2 = 619, kArrOff3 = 622;

struct MirrorState { MirrorChoice* c; };
INT_PTR CALLBACK MirrorProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = reinterpret_cast<MirrorState*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        CheckRadioButton(dlg, kMirAxis, kMirAxis + 5, kMirAxis);
        CheckRadioButton(dlg, kMirClone, kMirClone + 3, kMirClone);
        EnableWindow(GetDlgItem(dlg, kMirClone + 3), FALSE);    // Reference: not available
        SetDlgItemTextW(dlg, kMirOffset, L"0.0");
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            for (int i = 0; i < 6; ++i) if (IsDlgButtonChecked(dlg, kMirAxis + i) == BST_CHECKED) st->c->axis = i;
            for (int i = 0; i < 3; ++i) if (IsDlgButtonChecked(dlg, kMirClone + i) == BST_CHECKED) st->c->clone = i;
            st->c->offset = Field(dlg, kMirOffset);
            st->c->ok = true;
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) { EndDialog(dlg, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}

struct ArrayState { ArrayChoice* c; };
INT_PTR CALLBACK ArrayProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = reinterpret_cast<ArrayState*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        for (int i = 0; i < 3; ++i) {
            SetDlgItemTextW(dlg, kArrMove + i, L"0.0");
            SetDlgItemTextW(dlg, kArrRot + i, L"0.0");
            SetDlgItemTextW(dlg, kArrScale + i, L"100.0");
            SetDlgItemTextW(dlg, kArrOff2 + i, L"0.0");
            SetDlgItemTextW(dlg, kArrOff3 + i, L"0.0");
        }
        SetDlgItemTextW(dlg, kArrCount1, L"10");
        SetDlgItemTextW(dlg, kArrCount2, L"1");
        SetDlgItemTextW(dlg, kArrCount3, L"1");
        CheckRadioButton(dlg, kArrCopy, kArrRef, kArrCopy);
        EnableWindow(GetDlgItem(dlg, kArrRef), FALSE);
        CheckRadioButton(dlg, kArr1D, kArr3D, kArr1D);
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            ArrayChoice& c = *st->c;
            auto vec = [&](WORD first) { return math::Vec3{Field(dlg, first), Field(dlg, static_cast<WORD>(first + 1)), Field(dlg, static_cast<WORD>(first + 2))}; };
            c.move = vec(kArrMove);
            c.rotate = vec(kArrRot);
            c.scale = vec(kArrScale);
            c.offset2 = vec(kArrOff2);
            c.offset3 = vec(kArrOff3);
            c.instance = IsDlgButtonChecked(dlg, kArrInstance) == BST_CHECKED;
            c.dimensions = IsDlgButtonChecked(dlg, kArr3D) == BST_CHECKED ? 3 : IsDlgButtonChecked(dlg, kArr2D) == BST_CHECKED ? 2 : 1;
            c.count1 = static_cast<int>(Field(dlg, kArrCount1));
            c.count2 = static_cast<int>(Field(dlg, kArrCount2));
            c.count3 = static_cast<int>(Field(dlg, kArrCount3));
            c.ok = true;
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) { EndDialog(dlg, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}
} // namespace

MirrorChoice ShowMirrorDialog(HWND parent, const std::wstring& system) {
    const std::wstring title = L"Mirror: " + system + L" Coordinates";
    Template t(title.c_str(), 140, 120);
    t.Add(L"Button", L"Mirror Axis:", 0xFFFF, BS_GROUPBOX, 5, 3, 130, 62);
    const wchar_t* axes[6] = {L"X", L"Y", L"Z", L"XY", L"YZ", L"ZX"};
    for (int i = 0; i < 6; ++i)
        t.Add(L"Button", axes[i], static_cast<WORD>(kMirAxis + i), BS_AUTORADIOBUTTON | (i == 0 ? WS_GROUP | WS_TABSTOP : 0),
              static_cast<short>(12 + 60 * (i / 3)), static_cast<short>(14 + 12 * (i % 3)), 40, 10);
    t.Add(L"Static", L"Offset:", 0xFFFF, SS_RIGHT, 10, 52, 30, 9);
    t.Add(L"Edit", L"", kMirOffset, ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP | WS_GROUP, 44, 50, 60, 11);
    t.Add(L"Button", L"Clone Selection:", 0xFFFF, BS_GROUPBOX, 5, 68, 130, 32);
    const wchar_t* clone[4] = {L"No Clone", L"Copy", L"Instance", L"Reference"};
    for (int i = 0; i < 4; ++i)
        t.Add(L"Button", clone[i], static_cast<WORD>(kMirClone + i), BS_AUTORADIOBUTTON | (i == 0 ? WS_GROUP | WS_TABSTOP : 0),
              static_cast<short>(12 + 60 * (i / 2)), static_cast<short>(78 + 11 * (i % 2)), 55, 10);
    t.Add(L"Button", L"OK", IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 26, 104, 50, 13);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP, 84, 104, 50, 13);
    MirrorChoice c;
    MirrorState st{&c};
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), t.Get(), parent, MirrorProc, reinterpret_cast<LPARAM>(&st));
    return c;
}

ArrayChoice ShowArrayDialog(HWND parent) {
    Template t(L"Array", 260, 170);
    t.Add(L"Button", L"Incremental", 0xFFFF, BS_GROUPBOX, 5, 3, 250, 54);
    const wchar_t* rows[3] = {L"Move", L"Rotate", L"Scale"};
    const WORD firsts[3] = {kArrMove, kArrRot, kArrScale};
    const wchar_t* xyz[3] = {L"X", L"Y", L"Z"};
    for (int c = 0; c < 3; ++c) t.Add(L"Static", xyz[c], 0xFFFF, SS_CENTER, static_cast<short>(50 + 64 * c), 12, 50, 8);
    for (int r = 0; r < 3; ++r) {
        const short y = static_cast<short>(22 + 11 * r);
        t.Add(L"Static", rows[r], 0xFFFF, SS_RIGHT, 8, static_cast<short>(y + 1), 38, 8);
        for (int c = 0; c < 3; ++c)
            t.Add(L"Edit", L"", static_cast<WORD>(firsts[r] + c), ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, static_cast<short>(50 + 64 * c), y, 58, 10);
    }
    t.Add(L"Button", L"Type of Object", 0xFFFF, BS_GROUPBOX, 5, 60, 70, 46);
    t.Add(L"Button", L"Copy", kArrCopy, BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 12, 71, 55, 10);
    t.Add(L"Button", L"Instance", kArrInstance, BS_AUTORADIOBUTTON, 12, 82, 55, 10);
    t.Add(L"Button", L"Reference", kArrRef, BS_AUTORADIOBUTTON, 12, 93, 55, 10);
    t.Add(L"Button", L"Array Dimensions", 0xFFFF, BS_GROUPBOX, 80, 60, 175, 84);
    t.Add(L"Static", L"Count", 0xFFFF, SS_LEFT, 115, 69, 30, 8);
    const WORD dims[3] = {kArr1D, kArr2D, kArr3D}, counts[3] = {kArrCount1, kArrCount2, kArrCount3};
    const wchar_t* dimNames[3] = {L"1D", L"2D", L"3D"};
    for (int d = 0; d < 3; ++d) {
        const short y = static_cast<short>(79 + 20 * d);
        t.Add(L"Button", dimNames[d], dims[d], BS_AUTORADIOBUTTON | (d == 0 ? WS_GROUP | WS_TABSTOP : 0), 86, y, 26, 10);
        t.Add(L"Edit", L"", counts[d], ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP | WS_GROUP, 115, y, 30, 10);
        if (d > 0)
            for (int c = 0; c < 3; ++c)
                t.Add(L"Edit", L"", static_cast<WORD>((d == 1 ? kArrOff2 : kArrOff3) + c), ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP,
                      static_cast<short>(150 + 34 * c), y, 30, 10);
    }
    t.Add(L"Button", L"OK", IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 150, 152, 50, 13);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP, 205, 152, 50, 13);
    ArrayChoice c;
    ArrayState st{&c};
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), t.Get(), parent, ArrayProc, reinterpret_cast<LPARAM>(&st));
    return c;
}

namespace {
constexpr WORD kAlPos = 701, kAlCur = 704, kAlTgt = 708, kAlOri = 712, kAlScale = 715;
struct AlignState { AlignChoice* c; };
INT_PTR CALLBACK AlignProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = reinterpret_cast<AlignState*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        CheckRadioButton(dlg, kAlCur, kAlCur + 3, kAlCur + 1);    // Center
        CheckRadioButton(dlg, kAlTgt, kAlTgt + 3, kAlTgt + 1);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            AlignChoice& c = *st->c;
            for (int i = 0; i < 3; ++i) {
                c.position[i] = IsDlgButtonChecked(dlg, kAlPos + i) == BST_CHECKED;
                c.orientation[i] = IsDlgButtonChecked(dlg, kAlOri + i) == BST_CHECKED;
                c.scale[i] = IsDlgButtonChecked(dlg, kAlScale + i) == BST_CHECKED;
            }
            for (int i = 0; i < 4; ++i) {
                if (IsDlgButtonChecked(dlg, kAlCur + i) == BST_CHECKED) c.current = i;
                if (IsDlgButtonChecked(dlg, kAlTgt + i) == BST_CHECKED) c.target = i;
            }
            c.ok = true;
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) { EndDialog(dlg, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}
} // namespace

AlignChoice ShowAlignDialog(HWND parent, const std::wstring& targetName) {
    const std::wstring title = L"Align Selection (" + targetName + L")";
    Template t(title.c_str(), 190, 176);
    t.Add(L"Button", L"Align Position (World):", 0xFFFF, BS_GROUPBOX, 5, 3, 180, 84);
    const wchar_t* axes[3] = {L"X Position", L"Y Position", L"Z Position"};
    for (int i = 0; i < 3; ++i)
        t.Add(L"Button", axes[i], static_cast<WORD>(kAlPos + i), BS_AUTOCHECKBOX | WS_TABSTOP, static_cast<short>(12 + 58 * i), 14, 55, 10);
    t.Add(L"Static", L"Current Object:", 0xFFFF, SS_LEFT, 12, 28, 80, 8);
    t.Add(L"Static", L"Target Object:", 0xFFFF, SS_LEFT, 100, 28, 80, 8);
    const wchar_t* points[4] = {L"Minimum", L"Center", L"Pivot Point", L"Maximum"};
    for (int i = 0; i < 4; ++i) {
        t.Add(L"Button", points[i], static_cast<WORD>(kAlCur + i), BS_AUTORADIOBUTTON | (i == 0 ? WS_GROUP | WS_TABSTOP : 0), 14, static_cast<short>(38 + 11 * i), 70, 10);
        t.Add(L"Button", points[i], static_cast<WORD>(kAlTgt + i), BS_AUTORADIOBUTTON | (i == 0 ? WS_GROUP | WS_TABSTOP : 0), 102, static_cast<short>(38 + 11 * i), 70, 10);
    }
    t.Add(L"Button", L"Align Orientation (Local):", 0xFFFF, BS_GROUPBOX, 5, 90, 180, 26);
    t.Add(L"Button", L"Match Scale:", 0xFFFF, BS_GROUPBOX, 5, 119, 180, 26);
    const wchar_t* axis[3] = {L"X Axis", L"Y Axis", L"Z Axis"};
    for (int i = 0; i < 3; ++i) {
        t.Add(L"Button", axis[i], static_cast<WORD>(kAlOri + i), BS_AUTOCHECKBOX | WS_TABSTOP | (i == 0 ? WS_GROUP : 0), static_cast<short>(12 + 58 * i), 101, 50, 10);
        t.Add(L"Button", axis[i], static_cast<WORD>(kAlScale + i), BS_AUTOCHECKBOX | WS_TABSTOP | (i == 0 ? WS_GROUP : 0), static_cast<short>(12 + 58 * i), 130, 50, 10);
    }
    t.Add(L"Button", L"OK", IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 79, 155, 50, 14);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP, 135, 155, 50, 14);
    AlignChoice c;
    AlignState st{&c};
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), t.Get(), parent, AlignProc, reinterpret_cast<LPARAM>(&st));
    return c;
}

std::wstring AskName(HWND parent, const std::wstring& title, const std::wstring& label, const std::wstring& suggested) {
    // The Detach dialog's mechanics with a plain name field: reuse its procedure's IDs.
    Template t(title.c_str(), 160, 46);
    t.Add(L"Static", label.c_str(), 0xFFFF, SS_LEFT, 7, 9, 44, 9);
    t.Add(L"Edit", L"", kDetachName, ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 53, 7, 100, 12);
    t.Add(L"Button", L"", kToElement, BS_AUTOCHECKBOX, 0, 0, 0, 0);       // unused (the procedure disables it)
    t.Add(L"Button", L"OK", IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP, 46, 26, 50, 14);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP, 103, 26, 50, 14);
    DetachChoice choice;
    DetachState st{&choice, suggested};
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), t.Get(), parent, DetachProc, reinterpret_cast<LPARAM>(&st));
    return choice.ok ? (choice.name.empty() ? suggested : choice.name) : std::wstring();
}

// ---- New Scene and Merge (11.2) ----------------------------------------------------------

namespace {
constexpr WORD kNewKeepHier = 801, kNewKeepObj = 802, kNewAll = 803, kNewIcon = 804;
INT_PTR CALLBACK NewSceneProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        CheckRadioButton(dlg, kNewKeepHier, kNewAll, kNewAll);            // New All (measured)
        SendDlgItemMessageW(dlg, kNewIcon, STM_SETICON, reinterpret_cast<WPARAM>(LoadIconW(nullptr, IDI_QUESTION)), 0);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            int* choice = reinterpret_cast<int*>(GetWindowLongPtrW(dlg, DWLP_USER));
            *choice = IsDlgButtonChecked(dlg, kNewKeepHier) == BST_CHECKED ? 1 : IsDlgButtonChecked(dlg, kNewKeepObj) == BST_CHECKED ? 2 : 3;
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) { EndDialog(dlg, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}

constexpr WORD kMgName = 901, kMgList = 902, kMgAll = 903, kMgNone = 904, kMgInvert = 905, kMgSubtree = 906, kMgCase = 907,
               kMgSelSubtree = 908, kMgSortA = 909, kMgTypes = 912, kMgTypesAll = 919, kMgTypesNone = 920, kMgTypesInvert = 921;
// List types: Geometry, Shapes, Lights, Cameras, Helpers, Space Warps, Groups (912..918).
struct MergeState {
    const std::vector<MergeItem>* items;
    std::vector<int> shown;          // list row -> item index
    std::vector<int> chosen;
};
void FillMergeList(HWND dlg, MergeState& st) {
    const HWND list = GetDlgItem(dlg, kMgList);
    // Keep what is selected, by item.
    std::vector<char> was(st.items->size(), 0);
    for (int r = 0; r < static_cast<int>(st.shown.size()); ++r)
        if (SendMessageW(list, LB_GETSEL, r, 0) > 0) was[static_cast<size_t>(st.shown[static_cast<size_t>(r)])] = 1;
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    st.shown.clear();
    const bool subtree = IsDlgButtonChecked(dlg, kMgSubtree) == BST_CHECKED;
    const bool geometry = IsDlgButtonChecked(dlg, kMgTypes) == BST_CHECKED, groups = IsDlgButtonChecked(dlg, kMgTypes + 6) == BST_CHECKED;
    for (size_t i = 0; i < st.items->size(); ++i) {
        const MergeItem& it = (*st.items)[i];
        if (it.group ? !groups : !geometry) continue;
        const std::wstring text = std::wstring(subtree ? static_cast<size_t>(it.depth) * 3 : 0, L' ') + it.name;
        const LRESULT row = SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        st.shown.push_back(static_cast<int>(i));
        if (was[i]) SendMessageW(list, LB_SETSEL, TRUE, row);
    }
}
INT_PTR CALLBACK MergeProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = reinterpret_cast<MergeState*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        st = reinterpret_cast<MergeState*>(lp);
        CheckDlgButton(dlg, kMgSubtree, BST_CHECKED);
        CheckRadioButton(dlg, kMgSortA, kMgSortA + 2, kMgSortA);
        for (WORD t = kMgTypes; t < kMgTypes + 7; ++t) CheckDlgButton(dlg, t, BST_CHECKED);
        FillMergeList(dlg, *st);
        return TRUE;
    case WM_COMMAND: {
        const WORD id = LOWORD(wp);
        const HWND list = GetDlgItem(dlg, kMgList);
        const int rows = static_cast<int>(SendMessageW(list, LB_GETCOUNT, 0, 0));
        if (id == kMgAll || id == kMgNone) { SendMessageW(list, LB_SETSEL, id == kMgAll, -1); return TRUE; }
        if (id == kMgInvert) {
            for (int r = 0; r < rows; ++r) SendMessageW(list, LB_SETSEL, SendMessageW(list, LB_GETSEL, r, 0) <= 0, r);
            return TRUE;
        }
        if (id == kMgTypesAll || id == kMgTypesNone || id == kMgTypesInvert) {
            for (WORD t = kMgTypes; t < kMgTypes + 7; ++t)
                CheckDlgButton(dlg, t, id == kMgTypesAll ? BST_CHECKED : id == kMgTypesNone ? BST_UNCHECKED
                                                         : (IsDlgButtonChecked(dlg, t) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED));
            FillMergeList(dlg, *st);
            return TRUE;
        }
        if ((id >= kMgTypes && id < kMgTypes + 7) || id == kMgSubtree) { FillMergeList(dlg, *st); return TRUE; }
        if (id == kMgName && HIWORD(wp) == EN_CHANGE) {
            // Typing selects the names that start with the text (Case Sensitive optional).
            wchar_t buf[256] = L"";
            GetDlgItemTextW(dlg, kMgName, buf, 256);
            std::wstring want = buf;
            const bool cs = IsDlgButtonChecked(dlg, kMgCase) == BST_CHECKED;
            auto fold = [&](std::wstring v) { if (!cs) for (auto& c : v) c = static_cast<wchar_t>(towlower(c)); return v; };
            want = fold(want);
            for (int r = 0; r < rows; ++r) {
                const std::wstring n = fold((*st->items)[static_cast<size_t>(st->shown[static_cast<size_t>(r)])].name);
                SendMessageW(list, LB_SETSEL, !want.empty() && n.compare(0, want.size(), want) == 0, r);
            }
            return TRUE;
        }
        if (id == kMgList && HIWORD(wp) == LBN_SELCHANGE && IsDlgButtonChecked(dlg, kMgSelSubtree) == BST_CHECKED) {
            // Select Subtree: a selected object's descendants are selected too.
            for (int r = 0; r < rows; ++r) {
                int item = st->shown[static_cast<size_t>(r)];
                for (int p = (*st->items)[static_cast<size_t>(item)].parent; p >= 0; p = (*st->items)[static_cast<size_t>(p)].parent)
                    for (int q = 0; q < rows; ++q)
                        if (st->shown[static_cast<size_t>(q)] == p && SendMessageW(list, LB_GETSEL, q, 0) > 0) SendMessageW(list, LB_SETSEL, TRUE, r);
            }
            return TRUE;
        }
        if (id == IDOK) {
            for (int r = 0; r < rows; ++r)
                if (SendMessageW(list, LB_GETSEL, r, 0) > 0) st->chosen.push_back(st->shown[static_cast<size_t>(r)]);
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (id == IDCANCEL) { EndDialog(dlg, IDCANCEL); return TRUE; }
        break;
    }
    }
    return FALSE;
}
} // namespace

int ShowNewSceneDialog(HWND parent) {
    // Measured (r3.1-dlg-new.png), in dialog units.
    Template t(L"New Scene", 170, 94);
    t.Add(L"Static", L"", kNewIcon, SS_ICON, 13, 20, 21, 20);
    t.Add(L"Button", L"Options:", 0xFFFF, BS_GROUPBOX, 40, 4, 121, 55);
    t.Add(L"Button", L"Keep Objects and Hierarchy", kNewKeepHier, BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 52, 18, 105, 10);
    t.Add(L"Button", L"Keep Objects", kNewKeepObj, BS_AUTORADIOBUTTON, 52, 32, 105, 10);
    t.Add(L"Button", L"New All", kNewAll, BS_AUTORADIOBUTTON, 52, 45, 105, 10);
    t.Add(L"Button", L"OK", IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 40, 71, 51, 14);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP, 97, 71, 51, 14);
    int choice = 0;
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), t.Get(), parent, NewSceneProc, reinterpret_cast<LPARAM>(&choice));
    return choice;
}

std::vector<int> ShowMergeDialog(HWND parent, const std::wstring& fileName, const std::vector<MergeItem>& items) {
    // Measured (r3.1-dlg-merge.png), in dialog units. NOTE: Sort is shown greyed, as in the
    // screenshot; the list keeps the file's order.
    const std::wstring title = L"Merge - " + fileName;
    Template t(title.c_str(), 265, 212);
    t.Add(L"Edit", L"", kMgName, ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 5, 10, 155, 11);
    t.Add(L"ListBox", L"", kMgList, LBS_EXTENDEDSEL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL | WS_BORDER | WS_TABSTOP, 5, 23, 155, 136);
    t.Add(L"Button", L"All", kMgAll, BS_PUSHBUTTON | WS_TABSTOP, 20, 164, 37, 11);
    t.Add(L"Button", L"None", kMgNone, BS_PUSHBUTTON | WS_TABSTOP, 62, 164, 37, 11);
    t.Add(L"Button", L"Invert", kMgInvert, BS_PUSHBUTTON | WS_TABSTOP, 104, 164, 37, 11);
    t.Add(L"Button", L"Display Subtree", kMgSubtree, BS_AUTOCHECKBOX | WS_TABSTOP, 13, 184, 68, 10);
    t.Add(L"Button", L"Case Sensitive", kMgCase, BS_AUTOCHECKBOX | WS_TABSTOP, 83, 184, 68, 10);
    t.Add(L"Button", L"Select Subtree", kMgSelSubtree, BS_AUTOCHECKBOX | WS_TABSTOP, 13, 197, 68, 10);
    t.Add(L"Button", L"Sort", 0xFFFF, BS_GROUPBOX | WS_DISABLED, 165, 5, 95, 47);
    const wchar_t* sorts[3] = {L"Alphabetical", L"By Type", L"By Color"};
    for (int i = 0; i < 3; ++i)
        t.Add(L"Button", sorts[i], static_cast<WORD>(kMgSortA + i), BS_AUTORADIOBUTTON | WS_DISABLED | (i == 0 ? WS_GROUP : 0), 175, static_cast<short>(17 + 11 * i), 70, 10);
    t.Add(L"Button", L"List types", 0xFFFF, BS_GROUPBOX, 165, 57, 95, 115);
    const wchar_t* types[7] = {L"Geometry", L"Shapes", L"Lights", L"Cameras", L"Helpers", L"Space Warps", L"Groups"};
    for (int i = 0; i < 7; ++i)
        t.Add(L"Button", types[i], static_cast<WORD>(kMgTypes + i), BS_AUTOCHECKBOX | WS_TABSTOP | (i == 0 ? WS_GROUP : 0), 175, static_cast<short>(70 + 14 * i), 55, 10);
    t.Add(L"Button", L"All", kMgTypesAll, BS_PUSHBUTTON | WS_TABSTOP, 227, 69, 27, 11);
    t.Add(L"Button", L"None", kMgTypesNone, BS_PUSHBUTTON | WS_TABSTOP, 227, 83, 27, 11);
    t.Add(L"Button", L"Invert", kMgTypesInvert, BS_PUSHBUTTON | WS_TABSTOP, 227, 97, 27, 11);
    t.Add(L"Button", L"OK", IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 170, 190, 48, 13);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP, 225, 190, 35, 13);
    MergeState st{&items, {}, {}};
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), t.Get(), parent, MergeProc, reinterpret_cast<LPARAM>(&st));
    return st.chosen;
}

DetachChoice ShowDetachDialog(HWND parent, const std::wstring& suggestedName) {
    Template t(L"Detach", 160, 70);
    t.Add(L"Static", L"Detach as:", 0xFFFF, SS_LEFT, 7, 9, 36, 9);
    t.Add(L"Edit", L"", kDetachName, ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 45, 7, 108, 12);
    t.Add(L"Button", L"Detach To Element", kToElement, BS_AUTOCHECKBOX | WS_TABSTOP, 7, 24, 90, 10);
    t.Add(L"Button", L"Detach As Clone", kAsClone, BS_AUTOCHECKBOX | WS_TABSTOP, 7, 36, 90, 10);
    t.Add(L"Button", L"OK", IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP, 46, 50, 50, 14);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP, 103, 50, 50, 14);
    DetachChoice choice;
    DetachState st{&choice, suggestedName};
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), t.Get(), parent, DetachProc, reinterpret_cast<LPARAM>(&st));
    return choice;
}

CloneChoice ShowCloneDialog(HWND parent, const std::wstring& suggestedName, bool singleObject) {
    // Measured (11.2, r3.1-dlg-clone.png), in dialog units; the Controller group is greyed
    // (maxlike has no animation controllers).
    Template t(L"Clone Options", 143, 113);
    t.Add(L"Button", L"Controller", 0xFFFF, BS_GROUPBOX | WS_DISABLED, 72, 5, 61, 42);
    t.Add(L"Button", L"Copy", 0xFFFF, BS_AUTORADIOBUTTON | WS_DISABLED | WS_GROUP, 85, 18, 40, 10);
    t.Add(L"Button", L"Instance", 0xFFFF, BS_AUTORADIOBUTTON | WS_DISABLED, 85, 31, 44, 10);
    t.Add(L"Button", L"Object", 0xFFFF, BS_GROUPBOX, 8, 5, 59, 56);
    t.Add(L"Button", L"Copy", kCopy, BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 20, 18, 40, 10);
    t.Add(L"Button", L"Instance", kInstance, BS_AUTORADIOBUTTON, 20, 31, 44, 10);
    t.Add(L"Button", L"Reference", kReference, BS_AUTORADIOBUTTON, 20, 44, 44, 10);
    t.Add(L"Static", L"Name:", 0xFFFF, SS_RIGHT, 20, 69, 26, 9);
    t.Add(L"Edit", L"", kName, ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP | WS_GROUP, 48, 67, 68, 12);
    t.Add(L"Button", L"OK", IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP, 9, 90, 41, 13);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP, 91, 90, 42, 13);
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
