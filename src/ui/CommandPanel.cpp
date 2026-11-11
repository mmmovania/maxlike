#include "ui/CommandPanel.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"
#include "win/App.h"
#include "../../res/resource.h"
#include <windowsx.h>
#include <commdlg.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace ui {

using namespace cmdpanel;

// The colour that joins the selected tab to the body (measured: rows 28-29 under the
// Create tab are #C0C0C0, not the face colour).
static constexpr theme::Rgb kTabJoin{0xC0, 0xC0, 0xC0};
// The pressed category button's interior (measured).
static constexpr theme::Rgb kPressedFace{0xE1, 0xE1, 0xE1};
static constexpr int kDropdownId = 1;

bool CommandPanel::Create(HWND parent) {
    icons_ = static_cast<HBITMAP>(LoadImageW(win::App::Instance(), MAKEINTRESOURCEW(IDB_PANEL_TAB_ICONS),
                                             IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    categoryIcons_ = static_cast<HBITMAP>(LoadImageW(win::App::Instance(), MAKEINTRESOURCEW(IDB_PANEL_CATEGORY_ICONS),
                                                     IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    CreateParams p;
    p.title = L"Command panel";
    p.style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    p.parent = parent;
    if (!win::Window::Create(p)) return false;
    // A real Win32 combo box: with classic controls it draws exactly R3's dropdown. Its
    // closed height comes from the font (21 px with MS Sans Serif 8); the height given
    // here is for the open list.
    dropdown_ = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                                Scale(kDropdown.left), Scale(kDropdown.top), Scale(kDropdown.right - kDropdown.left + 1),
                                Scale(200), Hwnd(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDropdownId)),
                                win::App::Instance(), nullptr);
    FillDropdown();
    // The Modify tab's combos (measured: stack list x 36..167, rows title + 21..+44, 24
    // rows closed; selection level x 78..167, 21 rows), placed with their rollout.
    stackCombo_ = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
                                  0, 0, Scale(132), Scale(200), Hwnd(),
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDropdownId + 1)), win::App::Instance(), nullptr);
    levelCombo_ = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_DISABLED | CBS_DROPDOWNLIST,
                                  0, 0, Scale(90), Scale(120), Hwnd(), nullptr, win::App::Instance(), nullptr);
    return true;
}

void CommandPanel::FillDropdown() {
    if (!dropdown_) return;
    SendMessageW(dropdown_, CB_RESETCONTENT, 0, 0);
    for (const std::wstring& item : Subcategories(category_))
        SendMessageW(dropdown_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
    SendMessageW(dropdown_, CB_SETCURSEL, 0, 0);
}

void CommandPanel::UpdateCreateControls() {
    if (dropdown_) ShowWindow(dropdown_, selected_ == kCreate ? SW_SHOW : SW_HIDE);
}

void CommandPanel::SetModifyInfo(const ModifyInfo& info) {
    EndTyping(true);
    modify_ = info;
    {
        // The rollouts below the stack: the base object's Parameters, or the modifier's.
        std::vector<params::Spec> specs;
        if (modify_.currentIsModifier)
            specs = params::ModifierRollouts(modify_.currentClass, modify_.subObjectOn ? modify_.subObjectLevel : 0, modify_.sliceMode);
        else if (!params::ParametersFor(modify_.baseType).rows.empty()) specs = {params::ParametersFor(modify_.baseType)};
        bool same = specs.size() == modSpecs_.size();
        for (size_t i = 0; same && i < specs.size(); ++i) same = specs[i].title == modSpecs_[i].title;
        modSpecs_ = specs;
        if (!same) {                         // new rollouts: their own open/closed defaults
            modOpen_.clear();
            for (const auto& sp : modSpecs_) modOpen_.push_back(sp.startsOpen);
        }
    }
    if (stackCombo_) {
        SendMessageW(stackCombo_, CB_RESETCONTENT, 0, 0);
        for (const std::wstring& e : modify_.stack) SendMessageW(stackCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(e.c_str()));
        SendMessageW(stackCombo_, CB_SETCURSEL, static_cast<WPARAM>(modify_.current), 0);
    }
    if (levelCombo_) {
        // Edit Mesh lists its levels (enabled); Bend shows "Gizmo" (NOTE: its gizmo
        // sub-object is not implemented, so the combo stays disabled there).
        SendMessageW(levelCombo_, CB_RESETCONTENT, 0, 0);
        if (!modify_.subObjectLevels.empty()) {
            for (const std::wstring& l : modify_.subObjectLevels)
                SendMessageW(levelCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(l.c_str()));
            SendMessageW(levelCombo_, CB_SETCURSEL, static_cast<WPARAM>(modify_.subObjectLevel), 0);
        } else if (modify_.currentIsModifier) {
            SendMessageW(levelCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Gizmo"));
            SendMessageW(levelCombo_, CB_SETCURSEL, 0, 0);
        }
        EnableWindow(levelCombo_, !modify_.subObjectLevels.empty());
    }
    scroll_ = std::clamp(scroll_, 0, rollup::MaxScroll(RolloutItems(), Container()));
    UpdateModifyControls();
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// The two combos live inside the Modifier Stack rollout, so they move with the column
// and hide when it is closed or scrolled out of the container.
void CommandPanel::UpdateModifyControls() {
    const int t = InModify() ? TitleTopOf(kRollStack) : kNoTitle;
    const rollup::Box k = Container();
    auto place = [&](HWND h, int x, int y, int itemHeight) {
        if (!h) return;
        const bool show = t != kNoTitle && y > k.top && y + 24 < k.bottom;
        if (show) {
            SetWindowPos(h, nullptr, Scale(x), Scale(y), 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            if (itemHeight > 0) SendMessageW(h, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), Scale(itemHeight));
        }
        ShowWindow(h, show ? SW_SHOW : SW_HIDE);
    };
    place(stackCombo_, 36, t + 21, 18);   // 24 rows closed (measured)
    place(levelCombo_, 78, t + 94, 0);
}

// ---------------------------------------------------------------- Modify tab
// Measured (r3.1-modify-box.png), title-relative rows (Modifiers title at 64):
//   "Use Pivot Points": checkbox x 17, title + 22 (disabled with one object)
//   More... x 17..70 and Sets x 80..133, rows +40..+57; an icon button x 141..161, +37..+57
//   ten buttons x 17..88 / 92..163, 20 rows, at +63, +86, +109, +131, +154
// More... has a raised edge with a dark-grey outer bottom and right (EDGE_RAISED: white
// and light top-left, shadow and dark-grey bottom-right); the others are drawn like the
// Create panel's buttons (white top-left, black outer and shadow inner bottom-right).
namespace {
constexpr int kModButtonTop[5] = {63, 86, 109, 131, 154};
constexpr int kModButtonLeft[2] = {17, 92}, kModButtonWidth = 72, kModButtonHeight = 20;
}

int CommandPanel::ModifierButtonAt(int x, int y) const {
    const int t = TitleTopOf(kRollModifiers);
    if (t == kNoTitle) return -1;
    for (int r = 0; r < 5; ++r)
        for (int c = 0; c < 2; ++c) {
            const int L = kModButtonLeft[c], T = t + kModButtonTop[r];
            if (x >= L && x < L + kModButtonWidth && y >= T && y < T + kModButtonHeight) return r * 2 + c;
        }
    // More..., Sets and the icon button report as 10, 11, 12.
    if (y >= t + 40 && y <= t + 57) {
        if (x >= 17 && x <= 70) return 10;
        if (x >= 80 && x <= 133) return 11;
    }
    if (y >= t + 37 && y <= t + 57 && x >= 141 && x <= 161) return 12;
    return -1;
}

void CommandPanel::PaintModifiers(HDC dc, int t) {
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const COLORREF hi = theme::kButtonHighlight.Ref(), sh = theme::kButtonShadow.Ref(), dk = RGB(0, 0, 0);
    const COLORREF light = theme::kLight.Ref(), dark = theme::kDarkShadow.Ref();
    auto button = [&](int L, int T, int R, int B, const wchar_t* label, bool enabled) {
        px(L, T, R, B, theme::Face());
        px(L, T, R - 1, T, hi);  px(L, T, L, B, hi);
        px(R, T, R, B, dk);      px(L + 1, B, R, B, dk);
        px(R - 1, T + 1, R - 1, B - 1, sh);
        px(L + 2, B - 1, R - 1, B - 1, sh);
        if (!label) return;
        SIZE size{};
        const int n = static_cast<int>(wcslen(label));
        GetTextExtentPoint32W(dc, label, n, &size);
        DrawStateW(dc, nullptr, nullptr, reinterpret_cast<LPARAM>(label), n,
                   Scale(L) + (Scale(R - L + 1) - size.cx) / 2, Scale(T) + (Scale(B - T + 1) - size.cy) / 2, 0, 0,
                   DST_TEXT | (enabled ? DSS_NORMAL : DSS_DISABLED));
    };
    // "Use Pivot Points" (disabled with one object selected).
    RECT box{Scale(17), Scale(t + 22), Scale(30), Scale(t + 35)};
    DrawFrameControl(dc, &box, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_INACTIVE);
    DrawStateW(dc, nullptr, nullptr, reinterpret_cast<LPARAM>(L"Use Pivot Points"), 16, Scale(36), Scale(t + 22), 0, 0,
               DST_TEXT | DSS_DISABLED);
    // More...: EDGE_RAISED colours.
    {
        const int L = 17, T = t + 40, R = 70, B = t + 57;
        px(L, T, R, B, theme::Face());
        px(L, T, R - 1, T, hi);           px(L, T, L, B - 1, hi);
        px(R, T, R, B, dark);             px(L, B, R, B, dark);
        px(L + 1, T + 1, R - 2, T + 1, light);  px(L + 1, T + 1, L + 1, B - 2, light);
        px(R - 1, T + 1, R - 1, B - 1, sh);     px(L + 1, B - 1, R - 1, B - 1, sh);
        RECT text{Scale(L), Scale(T), Scale(R + 1), Scale(B)};
        DrawTextW(dc, L"More...", -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    button(80, t + 40, 133, t + 57, L"Sets", true);
    button(141, t + 37, 161, t + 57, nullptr, true);
    // NOTE: the icon on this button (R3: configure the button set) is not drawn yet.
    static const wchar_t* kNames[10] = {L"Bend", L"Taper", L"Twist", L"Noise", L"Extrude",
                                        L"Lathe", L"UVW Map", L"Edit Patch", L"Mesh Select", L"Edit Spline"};
    for (int i = 0; i < 10; ++i) {
        const int L = kModButtonLeft[i % 2], T = t + kModButtonTop[i / 2];
        button(L, T, L + kModButtonWidth - 1, T + kModButtonHeight - 1, kNames[i], i != 4 && i != 5 && i != 9);
    }
}

// Modifier Stack (title at 250): a pin icon, the stack combo, five icon buttons at rows
// +52..+74 (the second, Show End Result, drawn pressed as in R3), "Selection Level:",
// the Sub-Object button x 12..74 at +94..+114 and the level combo.
// The icons are original drawings (NOTE: their exact positions are approximate).
int CommandPanel::StackButtonAt(int x, int y) const {
    const int t = TitleTopOf(kRollStack);
    if (t == kNoTitle || y < t + 52 || y > t + 74) return -1;
    static const int kLeft[5] = {14, 36, 72, 98, 136}, kRight[5] = {33, 59, 96, 122, 158};
    for (int i = 0; i < 5; ++i)
        if (x >= kLeft[i] && x <= kRight[i]) return i;
    return -1;
}

void CommandPanel::PaintStack(HDC dc, int t) {
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const COLORREF hi = theme::kButtonHighlight.Ref(), sh = theme::kButtonShadow.Ref(), dk = RGB(0, 0, 0);
    auto pattern = [&](int x, int y, std::initializer_list<const char*> rows, COLORREF c) {
        int r = 0;
        for (const char* row : rows) {
            for (int col = 0; row[col]; ++col) if (row[col] == '#') px(x + col, y + r, x + col, y + r, c);
            ++r;
        }
    };
    const bool modifier = modify_.currentIsModifier;
    // Pin (original drawing).
    pattern(15, t + 28, {"......##.......", "......####.....", "#############..", "......####.....", "......##......."}, dk);
    // Light bulb: on/off for the current modifier (grey for the base object).
    const COLORREF bulb = modifier ? (modify_.currentEnabled ? RGB(0xFF, 0xD8, 0x00) : sh) : sh;
    pattern(18, t + 55, {"..###..", ".#...#.", "#.....#", "#.....#", "#.....#", ".#...#.", "..#.#..", "..###..", "..###.."}, bulb);
    // Show End Result: pressed frame with two bars.
    px(36, t + 52, 58, t + 52, sh);  px(36, t + 52, 36, t + 74, sh);
    px(59, t + 52, 59, t + 74, hi);  px(37, t + 74, 59, t + 74, hi);
    px(44, t + 57, 45, t + 68, dk);  px(50, t + 57, 51, t + 68, dk);
    // Etched separators.
    for (int x : {66, 130}) { px(x, t + 55, x, t + 70, sh); px(x + 1, t + 55, x + 1, t + 70, hi); }
    // Make Unique (a split arrow), Remove (a cross), Edit Stack (stacked bars): original.
    pattern(77, t + 57, {"#.........#", ".#.......#.", "..#.....#..", "...#...#...", "....#.#....", ".....#.....", ".....#.....", ".....#....."}, modifier ? dk : sh);
    pattern(104, t + 57, {"#.......#", ".#.....#.", "..#...#..", "...#.#...", "....#....", "...#.#...", "..#...#..", ".#.....#.", "#.......#"}, modifier ? dk : sh);
    pattern(141, t + 56, {"###########", "...........", "###########", "...........", "###########", "...........", "###########"}, dk);
    // Selection Level.
    RECT label{Scale(13), Scale(t + 78), Scale(170), Scale(t + 92)};
    DrawTextW(dc, L"Selection Level:", -1, &label, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    {
        const int L = 12, T = t + 94, R = 74, B = t + 114;
        px(L, T, R, B, theme::Face());
        if (modify_.subObjectOn) {
            // Measured (r3.1-modify-editmesh-vertex.png): the checked-button outline around
            // pale yellow #F0F080.
            px(L, T, R - 1, T, sh);          px(L, T, L, B, sh);
            px(R, T, R, B, hi);              px(R - 1, T + 1, R - 1, B, hi);
            px(L + 1, B, R, B, hi);          px(L + 2, B - 1, R, B - 1, hi);
            px(L + 2, T + 2, R - 2, B - 2, RGB(0xF0, 0xF0, 0x80));
        } else {
            px(L, T, R - 1, T, hi);  px(L, T, L, B, hi);
            px(R, T, R, B, dk);      px(L + 1, B, R, B, dk);
            px(R - 1, T + 1, R - 1, B - 1, sh);
            px(L + 2, B - 1, R - 1, B - 1, sh);
        }
        SIZE size{};
        GetTextExtentPoint32W(dc, L"Sub-Object", 10, &size);
        DrawStateW(dc, nullptr, nullptr, reinterpret_cast<LPARAM>(L"Sub-Object"), 10,
                   Scale(L) + (Scale(R - L + 1) - size.cx) / 2, Scale(T) + (Scale(B - T + 1) - size.cy) / 2, 0, 0,
                   DST_TEXT | (modifier || !modify_.subObjectLevels.empty() ? DSS_NORMAL : DSS_DISABLED));
    }
}

bool CommandPanel::NameColorBoxes(Box& field, Box& swatch) const {
    if (InModify()) {
        field = {16, 35, 149, 54};        // measured (r3.1-modify-box.png)
        swatch = {152, 35, 168, 54};
        return true;
    }
    const int t = ShowsRollouts() ? TitleTopOf(kRollNameColor) : kNoTitle;
    if (t == kNoTitle) return false;
    field = NameFieldBox(t);
    swatch = SwatchBox(t);
    return true;
}

bool CommandPanel::ShowsRollouts() const {
    if (InModify()) return true;
    // NOTE: other categories and types have their own rollouts in R3 (later phases).
    if (selected_ != kCreate || category_ != kGeometry || !dropdown_) return false;
    return SendMessageW(dropdown_, CB_GETCURSEL, 0, 0) == 0;   // "Standard Primitives"
}

std::vector<int> CommandPanel::VisibleRollouts() const {
    if (InModify()) {
        std::vector<int> ids = {kRollModifiers, kRollStack};
        for (size_t i = 0; i < modSpecs_.size(); ++i) ids.push_back(kRollDynamic + static_cast<int>(i));
        return ids;
    }
    if (createType_ < 0) return {kRollObjectType, kRollNameColor};
    return {kRollObjectType, kRollNameColor, kRollCreationMethod, kRollKeyboardEntry, kRollParameters};
}

std::vector<rollup::Item> CommandPanel::RolloutItems() const {
    std::vector<rollup::Item> items;
    for (int id : VisibleRollouts()) {
        if (id >= kRollDynamic) {
            const size_t k = static_cast<size_t>(id - kRollDynamic);
            items.push_back({modSpecs_[k].contentHeight, modOpen_[k]});
            continue;
        }
        int content = 0;
        switch (id) {
        case kRollObjectType:     content = kObjectTypeContent; break;
        case kRollNameColor:      content = kNameColorContent; break;
        case kRollCreationMethod: content = params::kCreationMethodContent; break;
        case kRollKeyboardEntry:  content = keyboard_.fields.contentHeight; break;
        case kRollParameters:     content = spec_.contentHeight; break;
        case kRollModifiers:      content = 164; break;   // measured (Modify tab)
        case kRollStack:          content = 106; break;
        }
        items.push_back({content, open_[id]});
    }
    return items;
}

int CommandPanel::TitleTopOf(int id) const {
    const std::vector<int> ids = VisibleRollouts();
    const auto placed = rollup::Layout(RolloutItems(), scroll_, Container());
    for (size_t i = 0; i < ids.size(); ++i)
        if (ids[i] == id) return placed[i].open ? placed[i].titleTop : kNoTitle;
    return kNoTitle;
}

void CommandPanel::SetParameterSource(const scene::ParamBlock* block) {
    source_ = block;
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

void CommandPanel::SetObjectName(const std::wstring& name) {
    if (name == objectName_) return;
    objectName_ = name;
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

void CommandPanel::SetScroll(int scroll) {
    scroll = std::clamp(scroll, 0, rollup::MaxScroll(RolloutItems(), Container()));
    if (scroll == scroll_) return;
    scroll_ = scroll;
    InvalidateRect(Hwnd(), nullptr, FALSE);
    UpdateModifyControls();
}

int CommandPanel::TypeButtonAt(int x, int y) const {
    const int titleTop = ShowsRollouts() ? TitleTopOf(kRollObjectType) : kNoTitle;
    if (titleTop == kNoTitle) return -1;
    for (int i = 0; i < 10; ++i) {
        const Box b = TypeButtonBox(i, titleTop);
        if (x >= b.left && x <= b.right && y >= b.top && y <= b.bottom && y >= Container().top + 1 &&
            y <= Container().bottom - 1)
            return i;
    }
    return -1;
}

// The rollup, measured: a 1-px sunken container; each rollout an etched frame with a
// raised title button over its top edge (2-px highlight top and left, 2-px shadow
// bottom and right), "-" when open at x 17..20, 8 rows below the title's top.
void CommandPanel::PaintRollouts(HDC dc) {
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const COLORREF hi = theme::kButtonHighlight.Ref(), sh = theme::kButtonShadow.Ref();
    const rollup::Box k = Container();
    // With the creation rollouts showing, R3 leaves a 4-px column inside the right edge
    // (x 174..177): the container's top and bottom stop at x 173. Measured; NOTE: probably
    // a gutter for a scroll indicator, the exact condition is unverified.
    // In the Modify tab it appears when the column overflows (Edit Mesh; not with a box or
    // Bend) and holds the scroll indicator.
    const int maxScroll = rollup::MaxScroll(RolloutItems(), k);
    const bool indicator = InModify() && maxScroll > 0;
    const int gutter = (InModify() ? indicator : createType_ >= 0) ? 4 : 0;
    px(k.left, k.top, k.right - 1 - (indicator ? 0 : gutter), k.top, sh);  // sunken: shadow top and left,
    px(k.left, k.top, k.left, k.bottom, sh);              // (down to the bottom row, measured)
    px(k.right, k.top, k.right, k.bottom, hi);          // highlight right and bottom
    px(k.left + 1, k.bottom, k.right - 1 - gutter, k.bottom, hi);
    px(k.right, k.bottom, k.right, k.bottom, hi);
    if (indicator) {
        // The scroll indicator (measured, Edit Mesh unscrolled and scrolled by 270): rows
        // top+1 .. bottom-1 (H rows) in black, the visible part of the column in grey:
        // from top+1 + floor(scroll * H / C) to top+1 + round((scroll + H) * H / C) - 1,
        // C = H + the maximum scroll; the end rounds up (both screenshots: 527.6 -> 528,
        // 700.4 -> 701). Under the container's top line, which runs across.
        const rollup::Thumb thumb = rollup::ScrollThumb(k, scroll_, maxScroll);
        px(k.right - 4, k.top + 1, k.right - 1, k.bottom - 1, RGB(0, 0, 0));
        px(k.right - 4, thumb.first, k.right - 1, thumb.last, sh);
    }

    // Everything else stays inside the container.
    const int saved = SaveDC(dc);
    // Content stops one row above the bottom edge (measured: Edit Mesh's column, row 883 empty).
    IntersectClipRect(dc, Scale(k.left + 1), Scale(k.top + 1), Scale(k.right), Scale(k.bottom - 1));
    HGDIOBJ oldFont = SelectObject(dc, font_ ? font_ : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));

    static const wchar_t* kTitles[kRollCount] = {L"Object Type", L"Name and Color", L"Creation Method",
                                                 L"Keyboard Entry", L"Parameters", L"Modifiers", L"Modifier Stack"};
    const std::vector<int> ids = VisibleRollouts();
    const auto placed = rollup::Layout(RolloutItems(), scroll_, Container());
    for (size_t i = 0; i < placed.size(); ++i) {
        const rollup::Placed& r = placed[i];
        const int id = ids[i];
        {
            // Etched frame, measured: a shadow rectangle, then a highlight rectangle 1 px
            // down and right drawn over it (so the highlight wins where they cross; not
            // quite DrawEdge's EDGE_ETCHED). Its top edge stops 1 px short of the title.
            const int fl = rollup::kFrameLeft, fr = rollup::kFrameRight, ft = r.frameTop, fb = r.frameBottom;
            auto outline = [&](int l, int t, int rr, int bb, COLORREF c) {
                px(l, t, rr, t, c); px(l, bb, rr, bb, c); px(l, t, l, bb, c); px(rr, t, rr, bb, c);
            };
            outline(fl, ft, fr - 1, fb - 1, sh);
            outline(fl + 1, ft + 1, fr, fb, hi);
            // The frame stops 1 px short of the title on each side, where they overlap
            // (measured: open rollouts' top edge; a collapsed rollout's whole small frame).
            px(rollup::kTitleLeft - 1, ft, rollup::kTitleRight + 1, std::min(fb, r.titleBottom), theme::Face());
            if (r.open) {
                if (id == kRollObjectType) PaintObjectType(dc, r.titleTop);
                if (id == kRollNameColor) PaintNameAndColor(dc, r.titleTop);
                if (id == kRollCreationMethod) PaintCreationMethod(dc, r.titleTop);
                if (id == kRollParameters) PaintRows(dc, r.titleTop, 0);
                if (id >= kRollDynamic) PaintRows(dc, r.titleTop, id - kRollDynamic);
                if (id == kRollModifiers) PaintModifiers(dc, r.titleTop);
                if (id == kRollStack) PaintStack(dc, r.titleTop);
                if (id == kRollKeyboardEntry) PaintRows(dc, r.titleTop, -1);
            }
        }
        // Title button over the frame's top edge.
        const int L = rollup::kTitleLeft, R = rollup::kTitleRight, T = r.titleTop, B = r.titleBottom;
        px(L, T, R, B, theme::Face());
        px(L, T, R - 1, T, hi);  px(L, T + 1, R - 2, T + 1, hi);       // top, 2 px
        px(L, T, L, B, hi);      px(L + 1, T, L + 1, B - 1, hi);       // left, 2 px
        px(R, T, R, B, sh);      px(R - 1, T + 1, R - 1, B, sh);       // right, 2 px
        px(L + 1, B, R, B, sh);  px(L + 2, B - 1, R, B - 1, sh);       // bottom, 2 px
        if (r.open) {
            px(17, T + 8, 20, T + 8, RGB(0, 0, 0));                    // "-" (measured)
        } else {
            px(17, T + 8, 22, T + 8, RGB(0, 0, 0));                    // "+" (measured: Keyboard Entry)
            px(19, T + 5, 20, T + 11, RGB(0, 0, 0));
        }
        RECT text{Scale(L + 2), Scale(T + 2), Scale(R - 1), Scale(B - 1)};
        const wchar_t* title = id >= kRollDynamic ? modSpecs_[static_cast<size_t>(id - kRollDynamic)].title.c_str() : kTitles[id];
        DrawTextW(dc, title, -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    SelectObject(dc, oldFont);
    RestoreDC(dc, saved);
}

// Object Type for Standard Primitives, measured: the disabled AutoGrid checkbox, then
// ten 69 x 18 buttons (1-px highlight top and left; dark shadow right and bottom with a
// shadow line inside). A checked button looks pressed (NOTE: R3's checked look is
// unverified; the category row's pressed look is used).
void CommandPanel::PaintObjectType(HDC dc, int titleTop) {
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const COLORREF hi = theme::kButtonHighlight.Ref(), sh = theme::kButtonShadow.Ref(), dk = RGB(0, 0, 0);

    // AutoGrid: a classic disabled checkbox, text on its left drawn embossed.
    RECT box{Scale(kAutoGridBoxLeft), Scale(titleTop + kAutoGridTop), Scale(kAutoGridBoxLeft + 13), Scale(titleTop + kAutoGridTop + 13)};
    // Enabled while a primitive is active (measured: R3.1 screenshot of a Box being created).
    const bool autoGridOn = createType_ >= 0;
    DrawFrameControl(dc, &box, DFC_BUTTON, DFCS_BUTTONCHECK | (autoGridOn ? 0 : DFCS_INACTIVE));
    const wchar_t* autoGrid = L"AutoGrid";
    SIZE size{};
    GetTextExtentPoint32W(dc, autoGrid, 8, &size);
    DrawStateW(dc, nullptr, nullptr, reinterpret_cast<LPARAM>(autoGrid), 8,
               Scale(kAutoGridTextRight + 1) - size.cx, Scale(titleTop + kAutoGridTop + 13) - size.cy, 0, 0,
               DST_TEXT | (autoGridOn ? DSS_NORMAL : DSS_DISABLED));

    const auto names = StandardPrimitiveNames();
    for (int i = 0; i < static_cast<int>(names.size()); ++i) {
        const Box b = TypeButtonBox(i, titleTop);
        const int L = b.left, T = b.top, R = b.right, B = b.bottom;
        const bool checked = i == createType_;
        px(L, T, R, B, theme::Face());
        if (checked) {
            // Measured: pressed outline (1-px shadow top and left, 2-px highlight right
            // and bottom, a face-coloured inner line) around light green #C0FFC0.
            px(L, T, R - 1, T, sh);          px(L, T, L, B, sh);
            px(R, T, R, B, hi);              px(R - 1, T + 1, R - 1, B, hi);
            px(L + 1, B, R, B, hi);          px(L + 2, B - 1, R, B - 1, hi);
            px(L + 2, T + 2, R - 2, B - 2, RGB(0xC0, 0xFF, 0xC0));
        } else {
            px(L, T, R - 1, T, hi);  px(L, T, L, B, hi);               // highlight top and left
            px(R, T, R, B, dk);      px(L + 1, B, R, B, dk);           // dark shadow right and bottom
            px(R - 1, T + 1, R - 1, B - 1, sh);                        // shadow inside them
            px(L + 2, B - 1, R - 1, B - 1, sh);
        }
        RECT text{Scale(L) + (checked ? 1 : 0), Scale(T) + (checked ? 1 : 0), Scale(R + 1), Scale(B)};
        DrawTextW(dc, names[static_cast<size_t>(i)].c_str(), -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}

// Name and Color, measured with nothing selected:
//   name field x 15..146, rows +23..+42 (title-relative): outer 1-px sunken edge (shadow
//   top and left, highlight right and bottom); inside it black top and left, face right
//   and bottom; then white: one row on top, two columns on each side; face inside
//   (the field is disabled until an object is selected; its enabled look: Phase 6).
//   swatch x 150..166, same rows: 1-px sunken edge filled with the colour.
void CommandPanel::PaintNameAndColor(HDC dc, int titleTop) {
    PaintNameColorBoxes(dc, NameFieldBox(titleTop), SwatchBox(titleTop));
}

void CommandPanel::PaintNameColorBoxes(HDC dc, const Box& f, const Box& w) {
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const COLORREF hi = theme::kButtonHighlight.Ref(), sh = theme::kButtonShadow.Ref(), face = theme::Face();
    auto sunken = [&](const Box& b) {
        px(b.left, b.top, b.right - 1, b.top, sh);
        px(b.left, b.top, b.left, b.bottom, sh);
        px(b.right, b.top, b.right, b.bottom, hi);
        px(b.left + 1, b.bottom, b.right, b.bottom, hi);
    };
    sunken(f);
    px(f.left + 1, f.top + 1, f.right - 2, f.top + 1, RGB(0, 0, 0));      // black top
    px(f.left + 1, f.top + 1, f.left + 1, f.bottom - 1, RGB(0, 0, 0));    // black left
    px(f.right - 1, f.top + 1, f.right - 1, f.bottom - 1, face);          // face right
    px(f.left + 2, f.bottom - 1, f.right - 1, f.bottom - 1, face);        // face bottom
    px(f.left + 2, f.top + 2, f.right - 2, f.top + 2, hi);                // white top row
    px(f.left + 2, f.top + 2, f.left + 3, f.bottom - 2, hi);              // white left, 2 px
    px(f.right - 3, f.top + 2, f.right - 2, f.bottom - 2, hi);            // white right, 2 px
    if (objectName_.empty()) {
        px(f.left + 4, f.top + 3, f.right - 4, f.bottom - 2, face);       // disabled: face inside
    } else {
        // Enabled (measured, Box01): white inside, the name in black.
        px(f.left + 2, f.top + 2, f.right - 2, f.bottom - 2, hi);
        if (!(typing_ && typingRow_ == kNameRow)) {
            RECT text{Scale(f.left + 5), Scale(f.top + 2), Scale(f.right - 2), Scale(f.bottom - 1)};
            DrawTextW(dc, objectName_.c_str(), -1, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
    }

    sunken(w);
    const COLORREF c = RGB((objectColor_ >> 16) & 0xFF, (objectColor_ >> 8) & 0xFF, objectColor_ & 0xFF);
    px(w.left + 1, w.top + 1, w.right - 1, w.bottom - 1, c);
}

// Creation Method: two classic radio buttons (measured for Box at x 34 and 102, title + 22).
// A classic radio button, drawn from its measured 12 x 12 pattern (Wine's
// DrawFrameControl draws a different circle; this is Windows', pixel for pixel).
void CommandPanel::DrawRadio(HDC dc, int x, int y, bool checked, bool enabled) {
        // Drawn from the measured 12 x 12 pattern (Wine's DrawFrameControl draws a
        // different circle; this is the classic Windows radio button, pixel for pixel).
        static const char* const kRadio[12] = {
            "....ssss....", "..ssddddss..", ".sddWWWWddW.", ".sdWWWWWWlW.", "sdWWWWWWWWlW", "sdWWWWWWWWlW",
            "sdWWWWWWWWlW", "sdWWWWWWWWlW", ".sdWWWWWWlW.", ".sllWWWWllW.", "..WWllllWW..", "....WWWW...."};
        static const char* const kDot[4] = {"..kk..", ".kkkk.", ".kkkk.", "..kk.."};   // rows 4..7, columns 3..8
        for (int ry = 0; ry < 12; ++ry)
            for (int rx = 0; rx < 12; ++rx) {
                char ch = kRadio[ry][rx];
                // Disabled (measured: Edit Mesh's Normal radios): the white inside turns face.
                if (!enabled && ch == 'W' && ry >= 2 && ry <= 9 && rx >= 2 && rx <= 9) ch = '.';
                if (checked && ry >= 4 && ry < 8 && rx >= 3 && rx < 9 && kDot[ry - 4][rx - 3] == 'k') ch = enabled ? 'k' : 's';   // disabled: a grey dot
                COLORREF c;
                switch (ch) {
                case 's': c = theme::kButtonShadow.Ref(); break;
                case 'd': c = theme::kDarkShadow.Ref(); break;
                case 'W': c = theme::kButtonHighlight.Ref(); break;
                case 'l': c = theme::kLight.Ref(); break;
                case 'k': c = RGB(0, 0, 0); break;
                default: continue;
                }
                FillSolid(dc, {Scale(x + rx), Scale(y + ry), Scale(x + rx + 1), Scale(y + ry + 1)}, c);
            }
}

void CommandPanel::PaintCreationMethod(HDC dc, int titleTop) {
    const params::Methods m = params::CreationMethodsFor(createType_);
    const std::wstring labels[2] = {m.first, m.second};
    for (int i = 0; i < 2; ++i) {
        const int x = params::kMethodRadioLeft[i], y = titleTop + params::kMethodTop;
        DrawRadio(dc, x, y, method_ == i);
        RECT text{Scale(x + params::kRadioSize + 4), Scale(y - 2), Scale(x + 66), Scale(y + params::kRadioSize + 2)};
        DrawTextW(dc, labels[i].c_str(), -1, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}

// Spinner rows have one id across both lists: Parameters rows 0, 1, ...; Keyboard Entry
// rows from kKeyBase. These hide the difference: Parameters edits the object (through
// onParamChanged), Keyboard Entry keeps its own values until Create.
const params::Row& CommandPanel::RowAt(int id) const {
    if (id >= kKeyBase) return keyboard_.fields.rows[static_cast<size_t>(id - kKeyBase)];
    return SpecAt(id / 100).rows[static_cast<size_t>(id % 100)];
}

int CommandPanel::RowTitleTop(int id) const {
    return TitleTopOf(id >= kKeyBase ? kRollKeyboardEntry : RollIdForSlot(id / 100));
}

double CommandPanel::RowValue(int id) const {
    if (id >= kKeyBase) return keValues_[static_cast<size_t>(id - kKeyBase)];
    return ParamValue(RowAt(id));
}

void CommandPanel::RowChanged(int id, double value, bool final) {
    if (id >= kKeyBase) {
        keValues_[static_cast<size_t>(id - kKeyBase)] = value;
    } else if (onParamChanged && RowAt(id).paramIndex >= 0) {
        onParamChanged(RowAt(id).paramIndex, value, final);
    }
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

double CommandPanel::ParamValue(const params::Row& r) const {
    const scene::ParamBlock* block = InModify() ? modify_.params : source_;
    if (r.paramIndex < 0 || !block) return 0;
    return block->Float(static_cast<size_t>(r.paramIndex));
}

// Parameters, measured (Box): labels right-aligned to x 87; the number field x 90..143
// (1-px sunken edge, black inner top and left, face inner right and bottom, white
// inside); the arrows x 146..157 split into up (rows 0..7) and down (8..15), each a
// small raised button with a 5-px triangle; checkboxes: classic, 13 x 13 at x 18.
void CommandPanel::PaintRows(HDC dc, int titleTop, int slot) {
    const bool keyboard = slot < 0;
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const COLORREF hi = theme::kButtonHighlight.Ref(), sh = theme::kButtonShadow.Ref(), face = theme::Face();
    const COLORREF k = RGB(0, 0, 0);
    const std::vector<params::Row>& rows = keyboard ? keyboard_.fields.rows : SpecAt(slot).rows;
    // Group boxes (measured on Bend): an etched frame x 17..163 drawn as two outlines,
    // the top line cleared around the label (x 24..gapEnd), the label 4 rows above it.
    if (!keyboard)
        for (const params::Group& g : SpecAt(slot).groups) {
            const int top = titleTop + g.top, bottom = titleTop + g.bottom, L = g.left, R = g.right;
            px(L + 1, top + 1, R, top + 1, hi);  px(L + 1, top + 1, L + 1, bottom + 1, hi);
            px(R, top, R, bottom + 1, hi);       px(L, bottom + 1, R, bottom + 1, hi);   // right edge from the top row
            px(L, top, R - 1, top, sh);          px(L, top, L, bottom, sh);
            px(R - 1, top, R - 1, bottom, sh);   px(L, bottom, R - 1, bottom, sh);
            if (!g.label.empty()) {
                px(g.gapStart, top, g.gapEnd, top + 1, face);
                RECT text{Scale(g.labelLeft), Scale(top - 6), Scale(160), Scale(top + 7)};
                DrawTextW(dc, g.label.c_str(), -1, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            }
        }
    for (size_t i = 0; i < rows.size(); ++i) {
        const params::Row& row = rows[i];
        const int id = keyboard ? kKeyBase + static_cast<int>(i) : slot * 100 + static_cast<int>(i);
        const int T = titleTop + row.top;
        const double value = (id == spinRow_ && spinModel_) ? spinModel_->Value() : RowValue(id);
        auto text = [&](const std::wstring& t, int L, int T0, int R, int B, UINT align, bool enabled) {
            if (enabled) {
                RECT rc{Scale(L), Scale(T0), Scale(R), Scale(B)};
                DrawTextW(dc, t.c_str(), -1, &rc, align | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                return;
            }
            SIZE size{};
            GetTextExtentPoint32W(dc, t.c_str(), static_cast<int>(t.size()), &size);
            const int x = (align & DT_RIGHT) ? Scale(R) - size.cx : (align & DT_CENTER) ? (Scale(L) + Scale(R) - size.cx) / 2 : Scale(L);
            DrawStateW(dc, nullptr, nullptr, reinterpret_cast<LPARAM>(t.c_str()), static_cast<int>(t.size()), x,
                       (Scale(T0) + Scale(B) - size.cy) / 2, 0, 0, DST_TEXT | DSS_DISABLED);
        };
        if (row.kind == params::Kind::Button) {
            // Drawn like the Object Type buttons (measured: Hide, Create, Extrude, ...).
            const int L = row.left, R = row.right, B = T + row.height - 1;
            px(L, T, R, B, face);
            px(L, T, R - 1, T, hi);  px(L, T, L, B, hi);
            px(R, T, R, B, k);       px(L + 1, B, R, B, k);
            px(R - 1, T + 1, R - 1, B - 1, sh);  px(L + 2, B - 1, R - 1, B - 1, sh);
            text(row.label, L, T, R + 1, B, DT_CENTER, row.enabled);
            continue;
        }
        if (row.kind == params::Kind::Label) {
            std::wstring t = row.label;
            const size_t at = t.find(L"{selection}");
            if (at != std::wstring::npos) t.replace(at, 11, modify_.selectionText);
            // A label paints its background, as R3's static controls do (measured: "pixels"
            // hides the end of the Target spinner's arrows).
            if (!row.alignRight) px(row.left, T, row.right, T + row.height - 1, face);
            text(t, row.alignRight ? 10 : row.left, T - 2, row.right + 1, T + row.height, row.alignRight ? DT_RIGHT : DT_LEFT, row.enabled);
            continue;
        }
        if (row.kind == params::Kind::Levels) {
            // Five sub-object levels (Vertex, Edge, Face, Polygon, Element), 25 px apart from
            // x 27; the active one in a 1-px sunken frame (measured: x 27..50, 23 rows).
            // The icons are original drawings.
            const int active = modify_.subObjectOn ? modify_.subObjectLevel : -1;
            for (int o = 0; o < 5; ++o) {
                const int L = 27 + 25 * o, R = L + 23, B = T + 22;
                if (o == active) { px(L, T, R - 1, T, sh); px(L, T, L, B, sh); px(R, T, R, B, hi); px(L + 1, B, R, B, hi); }
                const int cx = L + 11, cy = T + 11;
                const COLORREF ic = RGB(0x20, 0x20, 0x90);
                switch (o) {
                case 0: for (int d : {-5, 5}) for (int e : {-4, 4}) px(cx + d - 1, cy + e - 1, cx + d + 1, cy + e + 1, ic); break;
                case 1: for (int j = 0; j <= 8; ++j) { px(cx - 5 + j, cy + 4, cx - 5 + j, cy + 4, ic); px(cx - 5 + j / 2, cy + 4 - j, cx - 5 + j / 2, cy + 4 - j, ic); px(cx + 4 - j / 2, cy + 4 - j, cx + 4 - j / 2, cy + 4 - j, ic); } break;
                case 2: for (int j = 0; j <= 8; ++j) px(cx - 4 + j / 2, cy + 4 - j, cx + 4 - j / 2, cy + 4 - j, ic); break;
                case 3: px(cx - 5, cy - 4, cx + 5, cy + 4, ic); break;
                default: px(cx - 5, cy - 2, cx + 2, cy + 5, ic); px(cx - 2, cy - 5, cx + 5, cy + 2, RGB(0x50, 0x50, 0xC0)); break;
                }
            }
            continue;
        }
        if (row.kind == params::Kind::Check) {
            RECT box{Scale(row.checkLeft), Scale(T), Scale(row.checkLeft + 13), Scale(T + 13)};
            DrawFrameControl(dc, &box, DFC_BUTTON, DFCS_BUTTONCHECK | (row.enabled ? 0 : DFCS_INACTIVE));
            if (value != 0) {
                // The classic tick, from its measured 7 x 7 pattern (Refine Ends, disabled:
                // grey); Wine's DrawFrameControl draws a different one.
                static const char* const kTick[7] = {"......#", ".....##", "#...###", "##.###.", "#####..", ".###...", "..#...."};
                for (int ty = 0; ty < 7; ++ty)
                    for (int tx = 0; tx < 7; ++tx)
                        if (kTick[ty][tx] == '#') px(row.checkLeft + 3 + tx, T + 3 + ty, row.checkLeft + 3 + tx, T + 3 + ty, row.enabled ? k : sh);
            }
            text(row.label, row.checkLeft + 19, T - 2, 170, T + 15, DT_LEFT, row.enabled);
            continue;
        }
        if (row.kind == params::Kind::Radio) {
            for (size_t o = 0; o < row.options.size() && o < row.optionLefts.size(); ++o) {
                DrawRadio(dc, row.optionLefts[o], T, static_cast<int>(std::lround(value)) == static_cast<int>(o), row.enabled);
                text(row.options[o], row.optionLefts[o] + 17, T - 2, row.optionLefts[o] + 40, T + 14, DT_LEFT, row.enabled);
            }
            continue;
        }
        if (!row.label.empty()) text(row.label, 10, T, row.LabelRight() + 1, T + params::kSpinnerHeight, DT_RIGHT, row.enabled);
        // Field.
        const int L = row.fieldLeft, R = row.FieldRight(), B = T + params::kSpinnerHeight - 1;
        px(L, T, R - 1, T, sh);        px(L, T, L, B, sh);
        px(R, T, R, B, hi);            px(L + 1, B, R, B, hi);
        px(L + 1, T + 1, R - 2, T + 1, k);   px(L + 1, T + 1, L + 1, B - 1, k);
        px(R - 1, T + 1, R - 1, B - 1, face); px(L + 2, B - 1, R - 1, B - 1, face);
        px(L + 2, T + 2, R - 2, B - 2, hi);
        if (!row.enabled) {
            // Disabled (measured: Planar Thresh): face inside, 2-px white margins left and right.
            px(L + 4, T + 2, R - 4, B - 2, face);
        }
        if (!(typing_ && typingRow_ == id)) {
            SpinnerModel shown(value, row.min, row.max, row.step, row.kind == params::Kind::Int, row.decimals);
            const std::wstring t = shown.Text();
            text(t, L + 4, T + 2, R - 1, B - 1, DT_LEFT, row.enabled);
        }
        // Arrows.
        const int A = row.ArrowsLeft(), Z = row.ArrowsRight();
        px(A, T, A, B, hi);
        px(Z, T, Z, B, k);
        // Up half: rows 0..7 (shadow on row 7). Down half: rows 8..14 (shadow on row 14),
        // because the dark bottom line takes row 15.
        for (int half = 0; half < 2; ++half) {
            const int t = T + half * 8, last = half == 0 ? T + 7 : T + 14;
            px(A + 1, t, Z - 1, t, hi);                  // top highlight
            px(A + 1, t + 1, Z - 2, last - 1, face);
            px(Z - 1, t + 1, Z - 1, last - 1, sh);       // right shadow
            px(A + 1, last, A + 1, last, face);
            px(A + 2, last, Z - 1, last, sh);            // bottom shadow
        }
        px(A + 1, B, Z, B, k);                           // bottom: dark
        px(A + 5, T + 3, A + 5, T + 3, k); px(A + 4, T + 4, A + 6, T + 4, k); px(A + 3, T + 5, A + 7, T + 5, k);     // up
        px(A + 3, T + 10, A + 7, T + 10, k); px(A + 4, T + 11, A + 6, T + 11, k); px(A + 5, T + 12, A + 5, T + 12, k); // down
    }
    if (keyboard) {
        // The Create button, drawn like the Object Type buttons. NOTE: position unverified.
        const int L = params::kCreateButtonLeft, R = L + params::kCreateButtonWidth - 1;
        const int T = titleTop + keyboard_.buttonTop, B = T + params::kCreateButtonHeight - 1;
        px(L, T, R, B, face);
        px(L, T, R - 1, T, hi);  px(L, T, L, B, hi);
        px(R, T, R, B, k);       px(L + 1, B, R, B, k);
        px(R - 1, T + 1, R - 1, B - 1, sh);  px(L + 2, B - 1, R - 1, B - 1, sh);
        RECT text{Scale(L), Scale(T), Scale(R + 1), Scale(B)};
        DrawTextW(dc, L"Create", -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}

CommandPanel::Part CommandPanel::ParamPartAt(int x, int y, int& row) const {
    row = -1;
    if (!ShowsRollouts()) return Part::None;
    // Slots to look in: the Modify tab's rollouts, or the Create panel's Parameters and
    // Keyboard Entry (slot -1).
    std::vector<int> slots;
    if (InModify()) for (size_t i = 0; i < modSpecs_.size(); ++i) slots.push_back(static_cast<int>(i));
    else slots = {0, -1};
    for (int slot : slots) {
        const int titleTop = TitleTopOf(slot < 0 ? kRollKeyboardEntry : RollIdForSlot(slot));
        if (titleTop == kNoTitle) continue;
        const std::vector<params::Row>& rows = slot < 0 ? keyboard_.fields.rows : SpecAt(slot).rows;
        for (size_t i = 0; i < rows.size(); ++i) {
            const params::Row& r = rows[i];
            const int id = slot < 0 ? kKeyBase + static_cast<int>(i) : slot * 100 + static_cast<int>(i);
            const int T = titleTop + r.top;
            if (!r.enabled || r.kind == params::Kind::Label) continue;
            if (r.kind == params::Kind::Button) {
                if (y >= T && y < T + r.height && x >= r.left && x <= r.right) { row = id; return Part::Button; }
                continue;
            }
            if (r.kind == params::Kind::Levels) {
                for (int o = 0; o < 5; ++o)
                    if (y >= T && y <= T + 22 && x >= 27 + 25 * o && x <= 50 + 25 * o) { row = id; radioOption_ = o; return Part::Level; }
                continue;
            }
            if (r.kind == params::Kind::Check) {
                if (y >= T && y < T + 13 && x >= r.checkLeft && x < 170) { row = id; return Part::Check; }
                continue;
            }
            if (r.kind == params::Kind::Radio) {
                for (size_t o = 0; o < r.optionLefts.size(); ++o)
                    if (y >= T - 2 && y < T + 14 && x >= r.optionLefts[o] && x < r.optionLefts[o] + 34) {
                        row = id;
                        radioOption_ = static_cast<int>(o);
                        return Part::Radio;
                    }
                continue;
            }
            if (y < T || y >= T + params::kSpinnerHeight) continue;
            if (x >= r.fieldLeft && x <= r.FieldRight()) { row = id; return Part::Field; }
            if (x >= r.ArrowsLeft() && x <= r.ArrowsRight()) { row = id; return y < T + 8 ? Part::Up : Part::Down; }
        }
    }
    return Part::None;
}

bool CommandPanel::OnCreateButton(int x, int y) const {
    const int titleTop = TitleTopOf(kRollKeyboardEntry);
    if (titleTop == kNoTitle) return false;
    const int T = titleTop + keyboard_.buttonTop;
    return x >= params::kCreateButtonLeft && x < params::kCreateButtonLeft + params::kCreateButtonWidth &&
           y >= T && y < T + params::kCreateButtonHeight;
}

// Typing: a borderless EDIT over the field's white area; Enter or leaving it commits,
// Esc cancels (NOTE: R3's typing behaviour details are unverified).
void CommandPanel::BeginTyping(int row) {
    EndTyping(true);
    const int titleTop = RowTitleTop(row);
    if (titleTop == kNoTitle) return;
    const params::Row& r = RowAt(row);
    const int T = titleTop + r.top;
    SpinnerModel m(RowValue(row), r.min, r.max, r.step, r.kind == params::Kind::Int, r.decimals);
    typing_ = CreateWindowExW(0, L"EDIT", m.Text().c_str(), WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                              Scale(r.fieldLeft + 2), Scale(T + 2), Scale(r.FieldRight() - r.fieldLeft - 3),
                              Scale(params::kSpinnerHeight - 4), Hwnd(), nullptr, win::App::Instance(), nullptr);
    if (!typing_) return;
    typingRow_ = row;
    if (font_) SendMessageW(typing_, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
    SetWindowLongPtrW(typing_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    editOldProc_ = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(typing_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&CommandPanel::EditProc)));
    SendMessageW(typing_, EM_SETSEL, 0, -1);
    SetFocus(typing_);
}

// Renaming: an EDIT over the name field's white area (only while it is enabled).
void CommandPanel::BeginNameTyping() {
    EndTyping(true);
    Box f{}, w{};
    if (!NameColorBoxes(f, w) || objectName_.empty()) return;
    typing_ = CreateWindowExW(0, L"EDIT", objectName_.c_str(), WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                              Scale(f.left + 4), Scale(f.top + 3), Scale(f.right - f.left - 7), Scale(f.bottom - f.top - 5),
                              Hwnd(), nullptr, win::App::Instance(), nullptr);
    if (!typing_) return;
    typingRow_ = kNameRow;
    if (font_) SendMessageW(typing_, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
    SetWindowLongPtrW(typing_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    editOldProc_ = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(typing_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&CommandPanel::EditProc)));
    SendMessageW(typing_, EM_SETSEL, 0, -1);
    SetFocus(typing_);
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

void CommandPanel::EndTyping(bool commit) {
    if (!typing_) return;
    HWND edit = typing_;
    const int row = typingRow_;
    typing_ = nullptr;                    // first: destroying the edit sends WM_KILLFOCUS
    typingRow_ = -1;
    if (commit && row == kNameRow) {
        wchar_t buf[256] = {};
        GetWindowTextW(edit, buf, 256);
        if (onNameChanged && *buf && objectName_ != buf) onNameChanged(buf);   // an empty name is ignored
    } else if (commit && row >= 0) {
        wchar_t buf[64] = {};
        GetWindowTextW(edit, buf, 64);
        const params::Row& r = RowAt(row);
        SpinnerModel m(RowValue(row), r.min, r.max, r.step, r.kind == params::Kind::Int, r.decimals);
        if (m.SetText(buf)) RowChanged(row, m.Value(), true);
    }
    DestroyWindow(edit);
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

LRESULT CALLBACK CommandPanel::EditProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    auto* self = reinterpret_cast<CommandPanel*>(GetWindowLongPtrW(h, GWLP_USERDATA));
    const WNDPROC old = self ? self->editOldProc_ : DefWindowProcW;
    if (self && self->typing_ == h) {
        if (msg == WM_KEYDOWN && (wp == VK_RETURN || wp == VK_ESCAPE)) { self->EndTyping(wp == VK_RETURN); return 0; }
        if (msg == WM_CHAR && (wp == VK_RETURN || wp == VK_ESCAPE)) return 0;   // no beep
        if (msg == WM_KILLFOCUS) { const LRESULT r = CallWindowProcW(old, h, msg, wp, lp); self->EndTyping(true); return r; }
    }
    return CallWindowProcW(old, h, msg, wp, lp);
}

void CommandPanel::SetCreateType(int type) {
    if (type >= 0) {
        if (selected_ != kCreate) SelectTab(kCreate);
        if (category_ != kGeometry) SelectCategory(kGeometry);
        if (dropdown_ && SendMessageW(dropdown_, CB_GETCURSEL, 0, 0) != 0) SendMessageW(dropdown_, CB_SETCURSEL, 0, 0);
    }
    EndTyping(true);
    createType_ = type;
    spec_ = params::ParametersFor(createType_);
    keyboard_ = params::KeyboardEntryFor(createType_);
    keValues_.assign(keyboard_.fields.rows.size(), 0.0);   // NOTE: R3's defaults unverified
    method_ = params::CreationMethodsFor(createType_).selected;
    scroll_ = std::clamp(scroll_, 0, rollup::MaxScroll(RolloutItems(), Container()));
    InvalidateRect(Hwnd(), nullptr, FALSE);
    if (onCreateTypeChanged) onCreateTypeChanged(createType_);
}

void CommandPanel::SetObjectColor(unsigned rgb) {
    objectColor_ = rgb & 0xFFFFFF;
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

// NOTE: R3 opens its own "Object Color" dialog (a fixed palette); until that is
// built, Windows' standard colour dialog stands in.
void CommandPanel::PickObjectColor() {
    static COLORREF custom[16] = {};
    CHOOSECOLORW cc{};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = Hwnd();
    cc.rgbResult = RGB((objectColor_ >> 16) & 0xFF, (objectColor_ >> 8) & 0xFF, objectColor_ & 0xFF);
    cc.lpCustColors = custom;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;
    if (!ChooseColorW(&cc)) return;
    SetObjectColor((GetRValue(cc.rgbResult) << 16) | (GetGValue(cc.rgbResult) << 8) | GetBValue(cc.rgbResult));
    if (onObjectColorChanged) onObjectColorChanged(objectColor_);
}

void CommandPanel::SelectCategory(int c) {
    if (c < 0 || c >= kCategoryCount || c == category_) return;
    category_ = c;
    FillDropdown();
    InvalidateRect(Hwnd(), nullptr, FALSE);
    if (onCategoryChanged) onCategoryChanged(c);
    if (onSubcategoryChanged) onSubcategoryChanged(Subcategories(c).front());
}

// Category row (Create tab only), measured: the pressed button (x 9..32, rows 37..59)
// has a 1-px shadow top and left, a 2-px highlight right and bottom, a face-coloured
// inner line along its top and left, and #E1E1E1 inside; its icon moves 1 px down and
// right. Unpressed buttons show only their icon. NOTE: the hover look is unverified.
void CommandPanel::PaintCategories(HDC dc) {
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const Box b = CategoryBox(category_);
    const int L = b.left, T = b.top, R = b.right, B = b.bottom;
    const COLORREF hi = theme::kButtonHighlight.Ref(), sh = theme::kButtonShadow.Ref();
    px(L, T, R - 1, T, sh);                 // top
    px(L, T, L, B, sh);                     // left
    px(R, T, R, B, hi);                     // right, 2 px
    px(R - 1, T + 1, R - 1, B, hi);
    px(L + 1, B, R, B, hi);                 // bottom, 2 px
    px(L + 2, B - 1, R, B - 1, hi);
    px(L + 1, T + 1, R - 2, T + 1, theme::Face());
    px(L + 1, T + 1, L + 1, B - 1, theme::Face());
    px(L + 2, T + 2, R - 2, B - 2, kPressedFace.Ref());

    if (categoryIcons_) {
        HDC idc = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(idc, categoryIcons_);
        for (int c = 0; c < kCategoryCount; ++c) {
            const bool pressed = c == category_;
            TransparentBlt(dc, Scale(CategoryIconLeft(c, pressed)), Scale(CategoryIconTop(pressed)),
                           Scale(kCategoryIconSize), Scale(kCategoryIconSize), idc, c * kCategoryIconSize, 0,
                           kCategoryIconSize, kCategoryIconSize, RGB(255, 0, 255));
        }
        SelectObject(idc, old);
        DeleteDC(idc);
    }
}

void CommandPanel::SelectTab(int tab) {
    if (tab < 0 || tab >= kTabCount || tab == selected_) return;
    selected_ = tab;
    EndTyping(true);
    if (tab == kCreate) spec_ = params::ParametersFor(createType_);
    scroll_ = 0;
    UpdateCreateControls();
    UpdateModifyControls();
    if (onTabSelected) onTabSelected(tab);
    InvalidateRect(Hwnd(), nullptr, FALSE);
    if (onTabChanged) onTabChanged(tab);
}

// Tabs, measured (x relative to a tab's left edge L and right edge R, inclusive):
//   unselected: row 3 highlight L+2..R-2; row 4 highlight L+1, light L+2..R-2, dark R-1;
//               rows 5..27 highlight L, light L+1, face, shadow R-1, dark R.
//   selected:   the same shape 2 px wider on each side and starting 2 rows higher, its
//               sides continuing to row 28; rows 28-29 between its sides are #C0C0C0,
//               joining it to the body.
//   body:       row 28 highlight, row 29 light across the panel (the frame draws the
//               left and right edges).
// The selected tab is drawn last, over its neighbours.
void CommandPanel::PaintTabs(HDC dc) {
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    const COLORREF hi = theme::kButtonHighlight.Ref(), lt = theme::kLight.Ref();
    const COLORREF sh = theme::kButtonShadow.Ref(), dk = theme::kDarkShadow.Ref();
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {  // inclusive, in 100 % units
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const int panelRight = rc.right > 0 ? (rc.right * 96 + static_cast<int>(Dpi()) - 1) / static_cast<int>(Dpi()) - 1 : 0;

    // Body top edge (rows 28-29), from x 1 to the frame's right edge (measured with the
    // Modify tab selected; with Create selected, its tab covers x 1).
    px(1, kBodyTop, panelRight - 3, kBodyTop, hi);
    px(2, kBodyTop + 1, panelRight - 3, kBodyTop + 1, lt);

    auto tab = [&](int t, bool selected) {
        const Box b = TabBox(t, selected);
        const int L = b.left, R = b.right, top = b.top;
        const int sideBottom = selected ? kBodyTop : kTabBottom;
        px(L + 2, top, R - 2, top, hi);
        px(L + 1, top + 1, L + 1, top + 1, hi);
        px(L + 2, top + 1, R - 2, top + 1, lt);
        px(R - 1, top + 1, R - 1, top + 1, dk);
        px(L, top + 2, L, sideBottom, hi);
        px(L + 1, top + 2, L + 1, sideBottom, lt);
        px(L + 2, top + 2, R - 2, kTabBottom, theme::Face());
        px(R - 1, top + 2, R - 1, sideBottom, sh);
        px(R, top + 2, R, sideBottom, dk);
        if (selected) {
            px(L + 2, kBodyTop, R - 2, kBodyTop, kTabJoin.Ref());
            // Row 29 covers the tab's own side lines too (measured with Modify selected:
            // x 29..60); x 0..1 belong to the panel's frame (Create selected: from x 2).
            px(std::max(L, 2), kBodyTop + 1, R - 1, kBodyTop + 1, kTabJoin.Ref());
        }
    };
    for (int t = 0; t < kTabCount; ++t)
        if (t != selected_) tab(t, false);
    tab(selected_, true);

    // Icons (the same place whether the tab is selected or not).
    if (icons_) {
        HDC idc = CreateCompatibleDC(dc);
        HGDIOBJ old = SelectObject(idc, icons_);
        for (int t = 0; t < kTabCount; ++t)
            TransparentBlt(dc, Scale(IconLeft(t)), Scale(kIconTop), Scale(kIconWidth), Scale(kIconHeight),
                           idc, t * kIconWidth, 0, kIconWidth, kIconHeight, RGB(255, 0, 255));
        SelectObject(idc, old);
        DeleteDC(idc);
    }
}

void CommandPanel::OnPaint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(Hwnd(), &ps);
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    FillSolid(dc, rc, theme::Face());
    DrawFrameEdge(dc, rc, Edge::CommandPanel);
    PaintTabs(dc);
    if (selected_ == kCreate) PaintCategories(dc);
    if (ShowsRollouts()) PaintRollouts(dc);
    if (InModify()) {
        Box f{}, w{};
        NameColorBoxes(f, w);
        PaintNameColorBoxes(dc, f, w);
    }
    EndPaint(Hwnd(), &ps);
}

LRESULT CommandPanel::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        OnPaint();
        return 0;
    case WM_LBUTTONDOWN: {
        // Hit-test in 100 % units.
        const int x = GET_X_LPARAM(lp) * 96 / static_cast<int>(Dpi());
        const int y = GET_Y_LPARAM(lp) * 96 / static_cast<int>(Dpi());
        const int t = TabAt(x, y, selected_);
        if (t >= 0) { SelectTab(t); return 0; }
        if (selected_ == kCreate) {
            const int c = CategoryAt(x, y);
            if (c >= 0) { SelectCategory(c); return 0; }
        }
        if (!ShowsRollouts()) return 0;
        EndTyping(true);
        const std::vector<int> ids = VisibleRollouts();
        const std::vector<rollup::Item> items = RolloutItems();
        // A title opens or closes its rollout.
        const int title = rollup::TitleAt(items, scroll_, x, y, Container());
        if (title >= 0) {
            const int id = ids[static_cast<size_t>(title)];
            if (id >= kRollDynamic) modOpen_[static_cast<size_t>(id - kRollDynamic)] = !modOpen_[static_cast<size_t>(id - kRollDynamic)];
            else open_[id] = !open_[id];
            scroll_ = std::clamp(scroll_, 0, rollup::MaxScroll(RolloutItems(), Container()));
            UpdateModifyControls();
            InvalidateRect(Hwnd(), nullptr, FALSE);
            return 0;
        }
        // The colour swatch opens the colour picker; the name field (when enabled) takes typing.
        {
            Box f{}, w{};
            if (NameColorBoxes(f, w)) {
                if (x >= w.left && x <= w.right && y >= w.top && y <= w.bottom) { PickObjectColor(); return 0; }
                if (x >= f.left && x <= f.right && y >= f.top && y <= f.bottom) { BeginNameTyping(); return 0; }
            }
        }
        // The Modify tab's buttons.
        if (InModify()) {
            // ModifierButtonAt: the ten modifier buttons, then More... (10), Sets (11) and
            // the icon button (12).
            static const wchar_t* kNames[13] = {L"Bend", L"Taper", L"Twist", L"Noise", L"Extrude",
                                                L"Lathe", L"UVW Map", L"Edit Patch", L"Mesh Select", L"Edit Spline",
                                                L"More...", L"Sets", L"Configure Button Sets"};
            const int b = ModifierButtonAt(x, y);
            if (b >= 0) {
                if (b != 4 && b != 5 && b != 9 && onModifierButton) onModifierButton(kNames[b]);   // 4, 5, 9: disabled
                return 0;
            }
            const int s = StackButtonAt(x, y);
            if (s >= 0) { if (onStackButton) onStackButton(s); return 0; }
            const int st = TitleTopOf(kRollStack);
            if (st != kNoTitle && x >= 12 && x <= 74 && y >= st + 94 && y <= st + 114 && !modify_.subObjectLevels.empty()) {
                if (onSubObject) onSubObject(!modify_.subObjectOn);
                return 0;
            }
        }
        // A primitive button checks (or unchecks) its type.
        const int type = TypeButtonAt(x, y);
        if (type >= 0) {
            SetCreateType(type == createType_ ? -1 : type);
            return 0;
        }
        // Creation Method radios.
        const int methodTop = TitleTopOf(kRollCreationMethod);
        if (methodTop != kNoTitle && y >= methodTop + params::kMethodTop - 2 && y <= methodTop + params::kMethodTop + 13) {
            for (int i = 0; i < 2; ++i)
                if (x >= params::kMethodRadioLeft[i] && x < params::kMethodRadioLeft[i] + 62 && method_ != i) {
                    method_ = i;
                    InvalidateRect(Hwnd(), nullptr, FALSE);
                    if (onCreationMethodChanged) onCreationMethodChanged(i);
                    return 0;
                }
        }
        // Parameters: checkboxes toggle; arrows step (click) or drag; the field takes typing.
        // Keyboard Entry's Create button.
        if (OnCreateButton(x, y)) {
            if (onKeyboardCreate) {
                std::vector<std::pair<int, double>> sizes;
                double pos[3] = {0, 0, 0};
                for (size_t i = 0; i < keyboard_.fields.rows.size(); ++i) {
                    const int index = keyboard_.fields.rows[i].paramIndex;
                    if (index == params::kPosX) pos[0] = keValues_[i];
                    else if (index == params::kPosY) pos[1] = keValues_[i];
                    else if (index == params::kPosZ) pos[2] = keValues_[i];
                    else sizes.push_back({index, keValues_[i]});
                }
                onKeyboardCreate(pos[0], pos[1], pos[2], sizes);
            }
            return 0;
        }
        int row = -1;
        const Part part = ParamPartAt(x, y, row);
        if (part != Part::None) {
            const params::Row& r = RowAt(row);
            if (part == Part::Button) {
                if (onRolloutButton) onRolloutButton(r.label);
            } else if (part == Part::Level) {
                if (onSubObjectLevel) onSubObjectLevel(radioOption_);
            } else if (part == Part::Check) {
                RowChanged(row, RowValue(row) != 0 ? 0 : 1, true);
            } else if (part == Part::Radio) {
                if (static_cast<int>(std::lround(RowValue(row))) != radioOption_) RowChanged(row, radioOption_, true);
            } else if (part == Part::Field) {
                BeginTyping(row);
            } else {
                spinRow_ = row;
                spinPart_ = part;
                spinPressY_ = GET_Y_LPARAM(lp);
                spinMoved_ = false;
                spinModel_ = std::make_unique<SpinnerModel>(RowValue(row), r.min, r.max, r.step,
                                                            r.kind == params::Kind::Int, r.decimals);
                spinModel_->BeginDrag();
                SetCapture(Hwnd());
            }
            return 0;
        }
        // Anywhere else in the rollup: drag to scroll, if there is anything to scroll.
        if (rollup::MaxScroll(items, Container()) > 0 && y > Container().top && y < Container().bottom) {
            dragging_ = true;
            dragStartY_ = GET_Y_LPARAM(lp);
            dragStartScroll_ = scroll_;
            SetCapture(Hwnd());
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if (spinModel_) {
            // Dragging a spinner: vertical movement changes the value (preview).
            const int up = (spinPressY_ - GET_Y_LPARAM(lp)) * 96 / static_cast<int>(Dpi());
            if (!spinMoved_ && std::abs(up) < 2) return 0;
            spinMoved_ = true;
            spinModel_->DragTo(up, (wp & MK_CONTROL) != 0, GetKeyState(VK_MENU) < 0);
            RowChanged(spinRow_, spinModel_->Value(), false);
            return 0;
        }
        if (dragging_) SetScroll(dragStartScroll_ - (GET_Y_LPARAM(lp) - dragStartY_) * 96 / static_cast<int>(Dpi()));
        return 0;
    case WM_LBUTTONUP:
        if (spinModel_) {
            const int id = spinRow_;
            if (!spinMoved_) {                      // a click: one step
                spinModel_->CancelDrag();
                spinModel_->StepBy(spinPart_ == Part::Up ? 1 : -1);
            } else {
                spinModel_->EndDrag();
            }
            const double v = spinModel_->Value();
            spinModel_.reset();
            spinRow_ = -1;
            ReleaseCapture();
            RowChanged(id, v, true);
            return 0;
        }
        if (dragging_) ReleaseCapture();
        return 0;
    case WM_RBUTTONDOWN: {
        const int x = GET_X_LPARAM(lp) * 96 / static_cast<int>(Dpi());
        const int y = GET_Y_LPARAM(lp) * 96 / static_cast<int>(Dpi());
        if (spinModel_) {                           // right-click while dragging: cancel
            const int id = spinRow_;
            spinModel_->CancelDrag();
            const double v = spinModel_->Value();
            spinModel_.reset();
            spinRow_ = -1;
            ReleaseCapture();
            RowChanged(id, v, true);
            return 0;
        }
        int row = -1;
        const Part part = ParamPartAt(x, y, row);
        if (part == Part::Up || part == Part::Down) {   // right-click on the arrows: reset
            const params::Row& r = RowAt(row);
            SpinnerModel m(RowValue(row), r.min, r.max, r.step, r.kind == params::Kind::Int, r.decimals);
            m.Reset();
            RowChanged(row, m.Value(), true);
        }
        return 0;
    }
    case WM_CAPTURECHANGED:
        dragging_ = false;
        return 0;
    case WM_MOUSEWHEEL:
        if (ShowsRollouts()) SetScroll(scroll_ - GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA * 30);   // NOTE: step unverified
        return 0;
    case WM_SETCURSOR:
        // The hand shows where the rollup can be dragged (R3: over its empty space).
        if (LOWORD(lp) == HTCLIENT && ShowsRollouts() && rollup::MaxScroll(RolloutItems(), Container()) > 0) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(Hwnd(), &pt);
            const int x = pt.x * 96 / static_cast<int>(Dpi()), y = pt.y * 96 / static_cast<int>(Dpi());
            int row = -1;
            if (dragging_ || (y > Container().top && y < Container().bottom &&
                              rollup::TitleAt(RolloutItems(), scroll_, x, y, Container()) < 0 && TypeButtonAt(x, y) < 0 &&
                              ParamPartAt(x, y, row) == Part::None)) {
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            }
        }
        break;
    case WM_COMMAND:
        if (reinterpret_cast<HWND>(lp) == levelCombo_ && HIWORD(wp) == CBN_SELCHANGE) {
            const LRESULT i = SendMessageW(levelCombo_, CB_GETCURSEL, 0, 0);
            if (i >= 0 && onSubObjectLevel) onSubObjectLevel(static_cast<int>(i));
            return 0;
        }
        if (LOWORD(wp) == kDropdownId + 1 && HIWORD(wp) == CBN_SELCHANGE) {
            const LRESULT i = SendMessageW(stackCombo_, CB_GETCURSEL, 0, 0);
            if (i >= 0 && onStackSelect) onStackSelect(static_cast<int>(i));
            return 0;
        }
        if (LOWORD(wp) == kDropdownId && HIWORD(wp) == CBN_SELCHANGE && onSubcategoryChanged) {
            const LRESULT i = SendMessageW(dropdown_, CB_GETCURSEL, 0, 0);
            const auto items = Subcategories(category_);
            if (i >= 0 && static_cast<size_t>(i) < items.size()) onSubcategoryChanged(items[static_cast<size_t>(i)]);
            InvalidateRect(Hwnd(), nullptr, FALSE);   // the rollouts depend on the choice
        }
        return 0;
    case WM_SETFONT:
        font_ = reinterpret_cast<HFONT>(wp);
        // The combos use the UI font (it also sets their closed height).
        for (HWND combo : {dropdown_, stackCombo_, levelCombo_})
            if (combo) SendMessageW(combo, WM_SETFONT, wp, lp);
        UpdateModifyControls();
        return 0;
    case WM_GETFONT:
        return reinterpret_cast<LRESULT>(font_);
    case WM_DESTROY:
        if (icons_) { DeleteObject(icons_); icons_ = nullptr; }
        if (categoryIcons_) { DeleteObject(categoryIcons_); categoryIcons_ = nullptr; }
        break;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
