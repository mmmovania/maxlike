#include "ui/Dialogs.h"
#include <functional>
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
        for (WORD f = kAbsX; f <= kOffZ; ++f)                   // up = +1 (the default range is inverted)
            SendDlgItemMessageW(dlg, static_cast<WORD>(f + 100), UDM_SETRANGE32, static_cast<WPARAM>(-1000000), 1000000);
        return TRUE;
    case WM_NOTIFY:
        if (self && reinterpret_cast<NMHDR*>(lp)->code == UDN_DELTAPOS) {
            // The arrows (11.2b): a click steps the field by 1 and applies it at once; an
            // offset applies just that step. NOTE: R3's spinners also drag.
            const auto* ud = reinterpret_cast<NMUPDOWN*>(lp);
            const WORD field = static_cast<WORD>(ud->hdr.idFrom - 100);
            if (!IsWindowEnabled(GetDlgItem(dlg, field))) return TRUE;
            if (field <= kAbsZ) {
                SetDlgItemTextW(dlg, field, Num(Field(dlg, field) + ud->iDelta).c_str());
                const math::Vec3 v{Field(dlg, kAbsX), Field(dlg, kAbsY), Field(dlg, kAbsZ)};
                if (self->onApply) self->onApply(true, v);
            } else {
                math::Vec3 v{self->neutral_, self->neutral_, self->neutral_};
                const double step = ud->iDelta;
                if (field == kOffX) v.x += step; else if (field == kOffY) v.y += step; else v.z += step;
                if (self->onApply) self->onApply(false, v);
            }
            SetWindowLongPtrW(dlg, DWLP_MSGRESULT, 1);            // the control keeps no value of its own
            return TRUE;
        }
        break;
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
    // Measured (11.2b, r3.1-dlg-typein.png), in dialog units: offsets are in the view's
    // screen axes ("Offset:Screen").
    Template t(L"Move Transform Type-In", 192, 62);
    t.Add(L"Button", L"Absolute:World", 0xFFFF, BS_GROUPBOX, 4, 4, 90, 54);
    t.Add(L"Button", L"Offset:Screen", 0xFFFF, BS_GROUPBOX, 98, 4, 90, 54);
    const wchar_t* axes[3] = {L"X:", L"Y:", L"Z:"};
    for (int i = 0; i < 3; ++i) {
        const short y = static_cast<short>(16 + 13 * i);
        t.Add(L"Static", axes[i], 0xFFFF, SS_RIGHT, 8, static_cast<short>(y + 1), 12, 8);
        t.Add(L"Edit", L"", static_cast<WORD>(kAbsX + i), ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 23, y, 53, 9);
        t.Add(UPDOWN_CLASSW, L"", static_cast<WORD>(kAbsX + i + 100), 0, 78, y, 8, 9);
        t.Add(L"Static", axes[i], 0xFFFF, SS_RIGHT, 101, static_cast<short>(y + 1), 12, 8);
        t.Add(L"Edit", L"", static_cast<WORD>(kOffX + i), ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 116, y, 53, 9);
        t.Add(UPDOWN_CLASSW, L"", static_cast<WORD>(kOffX + i + 100), 0, 171, y, 8, 9);
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
constexpr WORD kMirAxis = 501, kMirOffset = 507, kMirClone = 508, kMirIK = 512;   // 501..506 axes, 508..511 clone options
constexpr WORD kArrInc = 601, kArrTot = 610, kArrLeft = 619, kArrRight = 622, kArrReorient = 625, kArrUniform = 626,
               kArrCopy = 627, kArrInstance = 628, kArrRef = 629, kArr1D = 630, kArrCount1 = 633, kArrOff2 = 636, kArrOff3 = 639,
               kArrTotal = 642, kArrReset = 643;      // kArrInc / kArrTot: 9 fields each (Move, Rotate, Scale x X, Y, Z)

struct MirrorState { MirrorChoice* c; };
INT_PTR CALLBACK MirrorProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = reinterpret_cast<MirrorState*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        CheckRadioButton(dlg, kMirAxis, kMirAxis + 5, kMirAxis);
        CheckRadioButton(dlg, kMirClone, kMirClone + 3, kMirClone);
        EnableWindow(GetDlgItem(dlg, kMirClone + 3), FALSE);    // Reference: maxlike has no references (R3: available)
        CheckDlgButton(dlg, kMirIK, BST_CHECKED);
        SetDlgItemTextW(dlg, kMirOffset, L"0.0");
        SendDlgItemMessageW(dlg, static_cast<WORD>(kMirOffset + 100), UDM_SETRANGE32, static_cast<WPARAM>(-1000000), 1000000);
        return TRUE;
    case WM_NOTIFY:
        if (reinterpret_cast<NMHDR*>(lp)->code == UDN_DELTAPOS) {
            const auto* ud = reinterpret_cast<NMUPDOWN*>(lp);
            wchar_t b[32]; std::swprintf(b, 32, L"%.1f", Field(dlg, kMirOffset) + ud->iDelta);
            SetDlgItemTextW(dlg, kMirOffset, b);
            SetWindowLongPtrW(dlg, DWLP_MSGRESULT, 1);
            return TRUE;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK) {
            for (int i = 0; i < 6; ++i) if (IsDlgButtonChecked(dlg, kMirAxis + i) == BST_CHECKED) st->c->axis = i;
            for (int i = 0; i < 3; ++i) if (IsDlgButtonChecked(dlg, kMirClone + i) == BST_CHECKED) st->c->clone = i;
            st->c->offset = Field(dlg, kMirOffset);
            st->c->ikLimits = IsDlgButtonChecked(dlg, kMirIK) == BST_CHECKED;
            st->c->ok = true;
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) { EndDialog(dlg, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}

// Array: the Incremental or the Totals side of each row is available, as its arrows choose.
void ArraySides(HWND dlg, const bool totals[3]) {
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c) {
            EnableWindow(GetDlgItem(dlg, kArrInc + r * 3 + c), !totals[r]);
            EnableWindow(GetDlgItem(dlg, kArrTot + r * 3 + c), totals[r]);
        }
}
void ArrayTotal(HWND dlg) {
    const int dims = IsDlgButtonChecked(dlg, kArr1D + 2) == BST_CHECKED ? 3 : IsDlgButtonChecked(dlg, kArr1D + 1) == BST_CHECKED ? 2 : 1;
    long long n = 1;
    for (int d = 0; d < dims; ++d) n *= std::max(1, static_cast<int>(Field(dlg, static_cast<WORD>(kArrCount1 + d))));
    SetDlgItemTextW(dlg, kArrTotal, std::to_wstring(n).c_str());
    for (int d = 1; d < 3; ++d) {
        EnableWindow(GetDlgItem(dlg, kArrCount1 + d), d < dims);
        for (int c = 0; c < 3; ++c) EnableWindow(GetDlgItem(dlg, (d == 1 ? kArrOff2 : kArrOff3) + c), d < dims);
    }
}
void ArrayDefaults(HWND dlg, bool totals[3]) {
    for (int r = 0; r < 3; ++r) {
        totals[r] = false;
        for (int c = 0; c < 3; ++c) {
            SetDlgItemTextW(dlg, kArrInc + r * 3 + c, r == 2 ? L"100.0" : L"0.0");
            SetDlgItemTextW(dlg, kArrTot + r * 3 + c, r == 2 ? L"100.0" : L"0.0");
        }
    }
    for (int c = 0; c < 3; ++c) { SetDlgItemTextW(dlg, kArrOff2 + c, L"0.0"); SetDlgItemTextW(dlg, kArrOff3 + c, L"0.0"); }
    SetDlgItemTextW(dlg, kArrCount1, L"10");
    SetDlgItemTextW(dlg, kArrCount1 + 1, L"1");
    SetDlgItemTextW(dlg, kArrCount1 + 2, L"1");
    CheckRadioButton(dlg, kArrCopy, kArrRef, kArrCopy);
    CheckRadioButton(dlg, kArr1D, kArr1D + 2, kArr1D);
    CheckDlgButton(dlg, kArrReorient, BST_CHECKED);
    CheckDlgButton(dlg, kArrUniform, BST_UNCHECKED);
    ArraySides(dlg, totals);
    ArrayTotal(dlg);
}
struct ArrayState { ArrayChoice* c; bool totals[3]; };
INT_PTR CALLBACK ArrayProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = reinterpret_cast<ArrayState*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        st = reinterpret_cast<ArrayState*>(lp);
        EnableWindow(GetDlgItem(dlg, kArrRef), FALSE);
        ArrayDefaults(dlg, st->totals);
        return TRUE;
    case WM_COMMAND: {
        const WORD id = LOWORD(wp);
        if (id >= kArrLeft && id < kArrLeft + 3) { st->totals[id - kArrLeft] = false; ArraySides(dlg, st->totals); return TRUE; }
        if (id >= kArrRight && id < kArrRight + 3) { st->totals[id - kArrRight] = true; ArraySides(dlg, st->totals); return TRUE; }
        if (id == kArrReset) { ArrayDefaults(dlg, st->totals); return TRUE; }
        if ((id >= kArr1D && id < kArr1D + 3) || ((id >= kArrCount1 && id < kArrCount1 + 3) && HIWORD(wp) == EN_CHANGE)) { ArrayTotal(dlg); return TRUE; }
        if (id == IDOK) {
            ArrayChoice& c = *st->c;
            auto vec = [&](WORD first) { return math::Vec3{Field(dlg, first), Field(dlg, static_cast<WORD>(first + 1)), Field(dlg, static_cast<WORD>(first + 2))}; };
            c.move = vec(kArrInc); c.rotate = vec(kArrInc + 3); c.scale = vec(kArrInc + 6);
            c.totalMove = vec(kArrTot); c.totalRotate = vec(kArrTot + 3); c.totalScale = vec(kArrTot + 6);
            for (int r = 0; r < 3; ++r) c.totals[r] = st->totals[r];
            c.offset2 = vec(kArrOff2); c.offset3 = vec(kArrOff3);
            c.instance = IsDlgButtonChecked(dlg, kArrInstance) == BST_CHECKED;
            c.dimensions = IsDlgButtonChecked(dlg, kArr1D + 2) == BST_CHECKED ? 3 : IsDlgButtonChecked(dlg, kArr1D + 1) == BST_CHECKED ? 2 : 1;
            c.count1 = static_cast<int>(Field(dlg, kArrCount1));
            c.count2 = static_cast<int>(Field(dlg, kArrCount1 + 1));
            c.count3 = static_cast<int>(Field(dlg, kArrCount1 + 2));
            c.reorient = IsDlgButtonChecked(dlg, kArrReorient) == BST_CHECKED;
            c.uniform = IsDlgButtonChecked(dlg, kArrUniform) == BST_CHECKED;
            c.ok = true;
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (id == IDCANCEL) { EndDialog(dlg, IDCANCEL); return TRUE; }
        break;
    }
    }
    return FALSE;
}

constexpr WORD kAlPos = 701, kAlCur = 704, kAlTgt = 708, kAlOri = 712, kAlScale = 715, kAlApply = 718;
struct AlignState { AlignChoice* c; const std::function<void(const AlignChoice&)>* onApply; };
void ReadAlign(HWND dlg, AlignChoice& c) {
    for (int i = 0; i < 3; ++i) {
        c.position[i] = IsDlgButtonChecked(dlg, kAlPos + i) == BST_CHECKED;
        c.orientation[i] = IsDlgButtonChecked(dlg, kAlOri + i) == BST_CHECKED;
        c.scale[i] = IsDlgButtonChecked(dlg, kAlScale + i) == BST_CHECKED;
    }
    for (int i = 0; i < 4; ++i) {
        if (IsDlgButtonChecked(dlg, kAlCur + i) == BST_CHECKED) c.current = i;
        if (IsDlgButtonChecked(dlg, kAlTgt + i) == BST_CHECKED) c.target = i;
    }
}
INT_PTR CALLBACK AlignProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    auto* st = reinterpret_cast<AlignState*>(GetWindowLongPtrW(dlg, DWLP_USER));
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dlg, DWLP_USER, lp);
        CheckRadioButton(dlg, kAlCur, kAlCur + 3, kAlCur + 1);    // Center
        CheckRadioButton(dlg, kAlTgt, kAlTgt + 3, kAlTgt + 1);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp) == kAlApply) {
            // Apply: this alignment now, as its own step; the dialog stays for another.
            AlignChoice c;
            ReadAlign(dlg, c);
            c.ok = true;
            if (*st->onApply) (*st->onApply)(c);
            for (WORD b = kAlPos; b < kAlPos + 3; ++b) CheckDlgButton(dlg, b, BST_UNCHECKED);
            for (WORD b = kAlOri; b < kAlScale + 3; ++b) CheckDlgButton(dlg, b, BST_UNCHECKED);
            return TRUE;
        }
        if (LOWORD(wp) == IDOK) {
            ReadAlign(dlg, *st->c);
            st->c->ok = true;
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
    // Measured (11.2b, r3.1-dlg-mirror.png), in dialog units. NOTE: Mirror IK Limits has no
    // effect (maxlike has no IK).
    const std::wstring title = L"Mirror: " + system + L" Coordinates";
    Template t(title.c_str(), 171, 174);
    t.Add(L"Button", L"Mirror Axis:", 0xFFFF, BS_GROUPBOX, 8, 6, 100, 71);
    const wchar_t* axes[6] = {L"X", L"Y", L"Z", L"XY", L"YZ", L"ZX"};
    for (int i = 0; i < 6; ++i)
        t.Add(L"Button", axes[i], static_cast<WORD>(kMirAxis + i), BS_AUTORADIOBUTTON | (i == 0 ? WS_GROUP | WS_TABSTOP : 0),
              static_cast<short>(29 + 36 * (i / 3)), static_cast<short>(21 + 13 * (i % 3)), 26, 10);
    t.Add(L"Static", L"Offset:", 0xFFFF, SS_RIGHT, 16, 61, 30, 9);
    t.Add(L"Edit", L"", kMirOffset, ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP | WS_GROUP, 48, 59, 34, 10);
    t.Add(UPDOWN_CLASSW, L"", static_cast<WORD>(kMirOffset + 100), UDS_ARROWKEYS, 83, 59, 7, 10);
    t.Add(L"Button", L"Clone Selection:", 0xFFFF, BS_GROUPBOX, 8, 81, 100, 73);
    const wchar_t* clone[4] = {L"No Clone", L"Copy", L"Instance", L"Reference"};
    for (int i = 0; i < 4; ++i)
        t.Add(L"Button", clone[i], static_cast<WORD>(kMirClone + i), BS_AUTORADIOBUTTON | (i == 0 ? WS_GROUP | WS_TABSTOP : 0),
              33, static_cast<short>(98 + 13 * i), 60, 10);
    t.Add(L"Button", L"Mirror IK Limits", kMirIK, BS_AUTOCHECKBOX | WS_TABSTOP | WS_GROUP, 9, 159, 80, 10);
    t.Add(L"Button", L"OK", IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 113, 10, 49, 13);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP, 113, 27, 49, 13);
    MirrorChoice c;
    MirrorState st{&c};
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), t.Get(), parent, MirrorProc, reinterpret_cast<LPARAM>(&st));
    return c;
}

ArrayChoice ShowArrayDialog(HWND parent, const std::wstring& system, const std::wstring& centre) {
    // Measured (11.2b, r3.1-dlg-array.png), in dialog units.
    Template t(L"Array", 450, 183);
    const std::wstring group = L"Array Transformation: " + system + L" Coordinates (" + centre + L")";
    t.Add(L"Button", group.c_str(), 0xFFFF, BS_GROUPBOX, 7, 6, 434, 86);
    t.Add(L"Static", L"Incremental", 0xFFFF, SS_CENTER, 50, 17, 60, 8);
    t.Add(L"Static", L"Totals", 0xFFFF, SS_CENTER, 255, 17, 60, 8);
    const wchar_t* xyz[3] = {L"X", L"Y", L"Z"};
    const short incCols[3] = {14, 61, 108}, totCols[3] = {219, 266, 313}, rows[3] = {39, 56, 74};
    for (int c = 0; c < 3; ++c) {
        t.Add(L"Static", xyz[c], 0xFFFF, SS_CENTER, incCols[c], 28, 31, 8);
        t.Add(L"Static", xyz[c], 0xFFFF, SS_CENTER, totCols[c], 28, 31, 8);
    }
    const wchar_t* names[3] = {L"Move", L"Rotate", L"Scale"};
    const wchar_t* units[3] = {L"units", L"degrees", L"percent"};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            t.Add(L"Edit", L"", static_cast<WORD>(kArrInc + r * 3 + c), ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, incCols[c], rows[r], 31, 10);
            t.Add(L"Edit", L"", static_cast<WORD>(kArrTot + r * 3 + c), ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, totCols[c], rows[r], 31, 10);
        }
        t.Add(L"Button", L"<", static_cast<WORD>(kArrLeft + r), BS_PUSHBUTTON, 154, rows[r], 9, 10);
        t.Add(L"Static", names[r], 0xFFFF, SS_CENTER, 165, static_cast<short>(rows[r] + 1), 30, 8);
        t.Add(L"Button", L">", static_cast<WORD>(kArrRight + r), BS_PUSHBUTTON, 199, rows[r], 9, 10);
        t.Add(L"Static", units[r], 0xFFFF, SS_LEFT, 349, static_cast<short>(rows[r] + 1), 32, 8);
    }
    t.Add(L"Button", L"Re-Orient", kArrReorient, BS_AUTOCHECKBOX | WS_TABSTOP, 388, 56, 48, 10);
    t.Add(L"Button", L"Uniform", kArrUniform, BS_AUTOCHECKBOX | WS_TABSTOP, 388, 74, 48, 10);
    t.Add(L"Button", L"Type of Object", 0xFFFF, BS_GROUPBOX, 7, 99, 75, 76);
    t.Add(L"Button", L"Copy", kArrCopy, BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 22, 120, 50, 10);
    t.Add(L"Button", L"Instance", kArrInstance, BS_AUTORADIOBUTTON, 22, 134, 50, 10);
    t.Add(L"Button", L"Reference", kArrRef, BS_AUTORADIOBUTTON, 22, 149, 50, 10);
    t.Add(L"Button", L"Array Dimensions", 0xFFFF, BS_GROUPBOX, 89, 99, 226, 76);
    t.Add(L"Static", L"Count", 0xFFFF, SS_CENTER, 127, 110, 23, 8);
    t.Add(L"Static", L"Incremental Row Offsets", 0xFFFF, SS_CENTER, 168, 110, 126, 8);
    for (int c = 0; c < 3; ++c) t.Add(L"Static", xyz[c], 0xFFFF, SS_CENTER, static_cast<short>(168 + 47 * c), 119, 31, 8);
    const wchar_t* dims[3] = {L"1D", L"2D", L"3D"};
    const short drows[3] = {123, 140, 158};
    for (int d = 0; d < 3; ++d) {
        t.Add(L"Button", dims[d], static_cast<WORD>(kArr1D + d), BS_AUTORADIOBUTTON | (d == 0 ? WS_GROUP | WS_TABSTOP : 0), 97, drows[d], 26, 10);
        t.Add(L"Edit", L"", static_cast<WORD>(kArrCount1 + d), ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP | WS_GROUP, 127, drows[d], 22, 10);
        if (d > 0)
            for (int c = 0; c < 3; ++c)
                t.Add(L"Edit", L"", static_cast<WORD>((d == 1 ? kArrOff2 : kArrOff3) + c), ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP,
                      static_cast<short>(168 + 47 * c), drows[d], 31, 10);
    }
    t.Add(L"Static", L"Total in Array:", 0xFFFF, SS_RIGHT, 337, 112, 52, 8);
    t.Add(L"Static", L"", kArrTotal, SS_LEFT | WS_BORDER, 392, 110, 30, 10);
    t.Add(L"Button", L"Reset All Parameters", kArrReset, BS_PUSHBUTTON | WS_TABSTOP, 337, 136, 100, 14);
    t.Add(L"Button", L"OK", IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP, 339, 160, 44, 14);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP, 391, 160, 46, 14);
    ArrayChoice c;
    ArrayState st{&c, {false, false, false}};
    DialogBoxIndirectParamW(GetModuleHandleW(nullptr), t.Get(), parent, ArrayProc, reinterpret_cast<LPARAM>(&st));
    return c;
}

AlignChoice ShowAlignDialog(HWND parent, const std::wstring& targetName, const std::wstring& system,
                            const std::function<void(const AlignChoice&)>& onApply) {
    // Measured (11.2b, r3.1-dlg-align.png), in dialog units.
    const std::wstring title = L"Align Selection (" + targetName + L")";
    Template t(title.c_str(), 235, 201);
    const std::wstring pos = L"Align Position (" + system + L"):";
    t.Add(L"Button", pos.c_str(), 0xFFFF, BS_GROUPBOX, 8, 6, 163, 113);
    const wchar_t* axes[3] = {L"X Position", L"Y Position", L"Z Position"};
    const short axisLefts[3] = {26, 77, 122};
    for (int i = 0; i < 3; ++i)
        t.Add(L"Button", axes[i], static_cast<WORD>(kAlPos + i), BS_AUTOCHECKBOX | WS_TABSTOP, axisLefts[i], 25, 46, 10);
    t.Add(L"Button", L"Current Object:", 0xFFFF, BS_GROUPBOX, 19, 40, 66, 69);
    t.Add(L"Button", L"Target Object:", 0xFFFF, BS_GROUPBOX, 93, 40, 66, 69);
    const wchar_t* points[4] = {L"Minimum", L"Center", L"Pivot Point", L"Maximum"};
    for (int i = 0; i < 4; ++i) {
        t.Add(L"Button", points[i], static_cast<WORD>(kAlCur + i), BS_AUTORADIOBUTTON | (i == 0 ? WS_GROUP | WS_TABSTOP : 0), 27, static_cast<short>(53 + 14 * i), 55, 10);
        t.Add(L"Button", points[i], static_cast<WORD>(kAlTgt + i), BS_AUTORADIOBUTTON | (i == 0 ? WS_GROUP | WS_TABSTOP : 0), 101, static_cast<short>(53 + 14 * i), 55, 10);
    }
    t.Add(L"Button", L"Align Orientation (Local):", 0xFFFF, BS_GROUPBOX, 8, 123, 163, 34);
    t.Add(L"Button", L"Match Scale:", 0xFFFF, BS_GROUPBOX, 8, 161, 163, 34);
    const wchar_t* axis[3] = {L"X Axis", L"Y Axis", L"Z Axis"};
    for (int i = 0; i < 3; ++i) {
        t.Add(L"Button", axis[i], static_cast<WORD>(kAlOri + i), BS_AUTOCHECKBOX | WS_TABSTOP | (i == 0 ? WS_GROUP : 0), axisLefts[i], 140, 42, 10);
        t.Add(L"Button", axis[i], static_cast<WORD>(kAlScale + i), BS_AUTOCHECKBOX | WS_TABSTOP | (i == 0 ? WS_GROUP : 0), axisLefts[i], 178, 42, 10);
    }
    t.Add(L"Button", L"OK", IDOK, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 177, 10, 48, 13);
    t.Add(L"Button", L"Cancel", IDCANCEL, BS_PUSHBUTTON | WS_TABSTOP, 177, 27, 48, 13);
    t.Add(L"Button", L"Apply", kAlApply, BS_PUSHBUTTON | WS_TABSTOP, 177, 52, 48, 13);
    AlignChoice c;
    AlignState st{&c, &onApply};
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
