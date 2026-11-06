#include "ui/CommandPanel.h"
#include "ui/Bevel.h"
#include "ui/Theme.h"
#include "win/App.h"
#include "../../res/resource.h"
#include <windowsx.h>
#include <commdlg.h>
#include <algorithm>
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

bool CommandPanel::ShowsRollouts() const {
    // NOTE: other categories and types have their own rollouts in R3 (later phases).
    if (selected_ != kCreate || category_ != kGeometry || !dropdown_) return false;
    return SendMessageW(dropdown_, CB_GETCURSEL, 0, 0) == 0;   // "Standard Primitives"
}

std::vector<int> CommandPanel::VisibleRollouts() const {
    if (createType_ < 0) return {kRollObjectType, kRollNameColor};
    return {kRollObjectType, kRollNameColor, kRollCreationMethod, kRollKeyboardEntry, kRollParameters};
}

std::vector<rollup::Item> CommandPanel::RolloutItems() const {
    std::vector<rollup::Item> items;
    for (int id : VisibleRollouts()) {
        int content = 0;
        switch (id) {
        case kRollObjectType:     content = kObjectTypeContent; break;
        case kRollNameColor:      content = kNameColorContent; break;
        case kRollCreationMethod: content = params::kCreationMethodContent; break;
        case kRollKeyboardEntry:  content = keyboard_.fields.contentHeight; break;
        case kRollParameters:     content = spec_.contentHeight; break;
        }
        items.push_back({content, open_[id]});
    }
    return items;
}

int CommandPanel::TitleTopOf(int id) const {
    const std::vector<int> ids = VisibleRollouts();
    const auto placed = rollup::Layout(RolloutItems(), scroll_);
    for (size_t i = 0; i < ids.size(); ++i)
        if (ids[i] == id) return placed[i].open ? placed[i].titleTop : -1;
    return -1;
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
    scroll = std::clamp(scroll, 0, rollup::MaxScroll(RolloutItems()));
    if (scroll == scroll_) return;
    scroll_ = scroll;
    InvalidateRect(Hwnd(), nullptr, FALSE);
}

int CommandPanel::TypeButtonAt(int x, int y) const {
    const int titleTop = ShowsRollouts() ? TitleTopOf(kRollObjectType) : -1;
    if (titleTop < 0) return -1;
    for (int i = 0; i < 10; ++i) {
        const Box b = TypeButtonBox(i, titleTop);
        if (x >= b.left && x <= b.right && y >= b.top && y <= b.bottom && y >= rollup::kContainer.top + 1 &&
            y <= rollup::kContainer.bottom - 1)
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
    const rollup::Box& k = rollup::kContainer;
    // With the creation rollouts showing, R3 leaves a 4-px column inside the right edge
    // (x 174..177): the container's top and bottom stop at x 173. Measured; NOTE: probably
    // a gutter for a scroll indicator, the exact condition is unverified.
    const int gutter = createType_ >= 0 ? 4 : 0;
    px(k.left, k.top, k.right - 1 - gutter, k.top, sh);  // sunken: shadow top and left,
    px(k.left, k.top, k.left, k.bottom, sh);              // (down to the bottom row, measured)
    px(k.right, k.top, k.right, k.bottom, hi);          // highlight right and bottom
    px(k.left + 1, k.bottom, k.right - 1 - gutter, k.bottom, hi);
    px(k.right, k.bottom, k.right, k.bottom, hi);

    // Everything else stays inside the container.
    const int saved = SaveDC(dc);
    IntersectClipRect(dc, Scale(k.left + 1), Scale(k.top + 1), Scale(k.right), Scale(k.bottom));
    HGDIOBJ oldFont = SelectObject(dc, font_ ? font_ : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));

    static const wchar_t* kTitles[kRollCount] = {L"Object Type", L"Name and Color", L"Creation Method",
                                                 L"Keyboard Entry", L"Parameters"};
    const std::vector<int> ids = VisibleRollouts();
    const auto placed = rollup::Layout(RolloutItems(), scroll_);
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
                if (id == kRollParameters) PaintRows(dc, r.titleTop, false);
                if (id == kRollKeyboardEntry) PaintRows(dc, r.titleTop, true);
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
        DrawTextW(dc, kTitles[id], -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
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
    const Box f = NameFieldBox(titleTop);
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

    const Box w = SwatchBox(titleTop);
    sunken(w);
    const COLORREF c = RGB((objectColor_ >> 16) & 0xFF, (objectColor_ >> 8) & 0xFF, objectColor_ & 0xFF);
    px(w.left + 1, w.top + 1, w.right - 1, w.bottom - 1, c);
}

// Creation Method: two classic radio buttons (measured for Box at x 34 and 102, title + 22).
void CommandPanel::PaintCreationMethod(HDC dc, int titleTop) {
    const params::Methods m = params::CreationMethodsFor(createType_);
    const std::wstring labels[2] = {m.first, m.second};
    for (int i = 0; i < 2; ++i) {
        const int x = params::kMethodRadioLeft[i], y = titleTop + params::kMethodTop;
        // Drawn from the measured 12 x 12 pattern (Wine's DrawFrameControl draws a
        // different circle; this is the classic Windows radio button, pixel for pixel).
        static const char* const kRadio[12] = {
            "....ssss....", "..ssddddss..", ".sddWWWWddW.", ".sdWWWWWWlW.", "sdWWWWWWWWlW", "sdWWWWWWWWlW",
            "sdWWWWWWWWlW", "sdWWWWWWWWlW", ".sdWWWWWWlW.", ".sllWWWWllW.", "..WWllllWW..", "....WWWW...."};
        static const char* const kDot[4] = {"..kk..", ".kkkk.", ".kkkk.", "..kk.."};   // rows 4..7, columns 3..8
        for (int ry = 0; ry < 12; ++ry)
            for (int rx = 0; rx < 12; ++rx) {
                char ch = kRadio[ry][rx];
                if (method_ == i && ry >= 4 && ry < 8 && rx >= 3 && rx < 9 && kDot[ry - 4][rx - 3] == 'k') ch = 'k';
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
        RECT text{Scale(x + params::kRadioSize + 4), Scale(y - 2), Scale(x + 66), Scale(y + params::kRadioSize + 2)};
        DrawTextW(dc, labels[i].c_str(), -1, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}

// Spinner rows have one id across both lists: Parameters rows 0, 1, ...; Keyboard Entry
// rows from kKeyBase. These hide the difference: Parameters edits the object (through
// onParamChanged), Keyboard Entry keeps its own values until Create.
const params::Row& CommandPanel::RowAt(int id) const {
    return id >= kKeyBase ? keyboard_.fields.rows[static_cast<size_t>(id - kKeyBase)] : spec_.rows[static_cast<size_t>(id)];
}

int CommandPanel::RowTitleTop(int id) const {
    return TitleTopOf(id >= kKeyBase ? kRollKeyboardEntry : kRollParameters);
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
    if (r.paramIndex < 0 || !source_) return 0;
    return source_->Float(static_cast<size_t>(r.paramIndex));
}

// Parameters, measured (Box): labels right-aligned to x 87; the number field x 90..143
// (1-px sunken edge, black inner top and left, face inner right and bottom, white
// inside); the arrows x 146..157 split into up (rows 0..7) and down (8..15), each a
// small raised button with a 5-px triangle; checkboxes: classic, 13 x 13 at x 18.
void CommandPanel::PaintRows(HDC dc, int titleTop, bool keyboard) {
    auto px = [&](int x0, int y0, int x1, int y1, COLORREF c) {
        if (x1 < x0 || y1 < y0) return;
        FillSolid(dc, {Scale(x0), Scale(y0), Scale(x1 + 1), Scale(y1 + 1)}, c);
    };
    const COLORREF hi = theme::kButtonHighlight.Ref(), sh = theme::kButtonShadow.Ref(), face = theme::Face();
    const COLORREF k = RGB(0, 0, 0);
    const std::vector<params::Row>& rows = keyboard ? keyboard_.fields.rows : spec_.rows;
    for (size_t i = 0; i < rows.size(); ++i) {
        const params::Row& row = rows[i];
        const int id = static_cast<int>(i) + (keyboard ? kKeyBase : 0);
        const int T = titleTop + row.top;
        const double value = (id == spinRow_ && spinModel_) ? spinModel_->Value() : RowValue(id);
        if (row.kind == params::Kind::Check) {
            RECT box{Scale(params::kCheckLeft), Scale(T), Scale(params::kCheckLeft + 13), Scale(T + 13)};
            DrawFrameControl(dc, &box, DFC_BUTTON, DFCS_BUTTONCHECK | (value != 0 ? DFCS_CHECKED : 0));
            RECT text{Scale(params::kCheckTextLeft), Scale(T - 2), Scale(170), Scale(T + 15)};
            DrawTextW(dc, row.label.c_str(), -1, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            continue;
        }
        RECT label{Scale(10), Scale(T), Scale(params::kLabelRight + 1), Scale(T + params::kSpinnerHeight)};
        DrawTextW(dc, row.label.c_str(), -1, &label, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        // Field.
        const int L = params::kFieldLeft, R = params::kFieldRight, B = T + params::kSpinnerHeight - 1;
        px(L, T, R - 1, T, sh);        px(L, T, L, B, sh);
        px(R, T, R, B, hi);            px(L + 1, B, R, B, hi);
        px(L + 1, T + 1, R - 2, T + 1, k);   px(L + 1, T + 1, L + 1, B - 1, k);
        px(R - 1, T + 1, R - 1, B - 1, face); px(L + 2, B - 1, R - 1, B - 1, face);
        px(L + 2, T + 2, R - 2, B - 2, hi);
        if (!(typing_ && typingRow_ == id)) {
            SpinnerModel shown(value, row.min, row.max, row.step, row.kind == params::Kind::Int, params::kFloatDecimals);
            const std::wstring text = shown.Text();
            RECT tr{Scale(L + 4), Scale(T + 2), Scale(R - 1), Scale(B - 1)};
            DrawTextW(dc, text.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
        // Arrows.
        const int A = params::kArrowsLeft, Z = params::kArrowsRight;
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
        px(151, T + 3, 151, T + 3, k); px(150, T + 4, 152, T + 4, k); px(149, T + 5, 153, T + 5, k);     // up
        px(149, T + 10, 153, T + 10, k); px(150, T + 11, 152, T + 11, k); px(151, T + 12, 151, T + 12, k); // down
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
    for (int list = 0; list < 2; ++list) {
        const bool keyboard = list == 1;
        const int titleTop = TitleTopOf(keyboard ? kRollKeyboardEntry : kRollParameters);
        if (titleTop < 0) continue;
        const std::vector<params::Row>& rows = keyboard ? keyboard_.fields.rows : spec_.rows;
        for (size_t i = 0; i < rows.size(); ++i) {
            const params::Row& r = rows[i];
            const int id = static_cast<int>(i) + (keyboard ? kKeyBase : 0);
            const int T = titleTop + r.top;
            if (r.kind == params::Kind::Check) {
                if (y >= T && y < T + 13 && x >= params::kCheckLeft && x < 170) { row = id; return Part::Check; }
                continue;
            }
            if (y < T || y >= T + params::kSpinnerHeight) continue;
            if (x >= params::kFieldLeft && x <= params::kFieldRight) { row = id; return Part::Field; }
            if (x >= params::kArrowsLeft && x <= params::kArrowsRight) { row = id; return y < T + 8 ? Part::Up : Part::Down; }
        }
    }
    return Part::None;
}

bool CommandPanel::OnCreateButton(int x, int y) const {
    const int titleTop = TitleTopOf(kRollKeyboardEntry);
    if (titleTop < 0) return false;
    const int T = titleTop + keyboard_.buttonTop;
    return x >= params::kCreateButtonLeft && x < params::kCreateButtonLeft + params::kCreateButtonWidth &&
           y >= T && y < T + params::kCreateButtonHeight;
}

// Typing: a borderless EDIT over the field's white area; Enter or leaving it commits,
// Esc cancels (NOTE: R3's typing behaviour details are unverified).
void CommandPanel::BeginTyping(int row) {
    EndTyping(true);
    const int titleTop = RowTitleTop(row);
    if (titleTop < 0) return;
    const params::Row& r = RowAt(row);
    const int T = titleTop + r.top;
    SpinnerModel m(RowValue(row), r.min, r.max, r.step, r.kind == params::Kind::Int, params::kFloatDecimals);
    typing_ = CreateWindowExW(0, L"EDIT", m.Text().c_str(), WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                              Scale(params::kFieldLeft + 2), Scale(T + 2), Scale(params::kFieldRight - params::kFieldLeft - 3),
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
    const int titleTop = TitleTopOf(kRollNameColor);
    if (titleTop < 0 || objectName_.empty()) return;
    const Box f = NameFieldBox(titleTop);
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
        SpinnerModel m(RowValue(row), r.min, r.max, r.step, r.kind == params::Kind::Int, params::kFloatDecimals);
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
    scroll_ = std::clamp(scroll_, 0, rollup::MaxScroll(RolloutItems()));
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
    UpdateCreateControls();
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

    // Body top edge (rows 28-29), from x 2 to the frame's right edge.
    px(2, kBodyTop, panelRight - 3, kBodyTop, hi);
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
            px(L + 2, kBodyTop + 1, R - 1, kBodyTop + 1, kTabJoin.Ref());
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
        const int title = rollup::TitleAt(items, scroll_, x, y);
        if (title >= 0) {
            const int id = ids[static_cast<size_t>(title)];
            open_[id] = !open_[id];
            scroll_ = std::clamp(scroll_, 0, rollup::MaxScroll(RolloutItems()));
            InvalidateRect(Hwnd(), nullptr, FALSE);
            return 0;
        }
        // The colour swatch opens the colour picker; the name field (when enabled) takes typing.
        const int nameTop = TitleTopOf(kRollNameColor);
        if (nameTop >= 0) {
            const Box w = SwatchBox(nameTop);
            if (x >= w.left && x <= w.right && y >= w.top && y <= w.bottom) { PickObjectColor(); return 0; }
            const Box f = NameFieldBox(nameTop);
            if (x >= f.left && x <= f.right && y >= f.top && y <= f.bottom) { BeginNameTyping(); return 0; }
        }
        // A primitive button checks (or unchecks) its type.
        const int type = TypeButtonAt(x, y);
        if (type >= 0) {
            SetCreateType(type == createType_ ? -1 : type);
            return 0;
        }
        // Creation Method radios.
        const int methodTop = TitleTopOf(kRollCreationMethod);
        if (methodTop >= 0 && y >= methodTop + params::kMethodTop - 2 && y <= methodTop + params::kMethodTop + 13) {
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
            if (part == Part::Check) {
                RowChanged(row, RowValue(row) != 0 ? 0 : 1, true);
            } else if (part == Part::Field) {
                BeginTyping(row);
            } else {
                spinRow_ = row;
                spinPart_ = part;
                spinPressY_ = GET_Y_LPARAM(lp);
                spinMoved_ = false;
                spinModel_ = std::make_unique<SpinnerModel>(RowValue(row), r.min, r.max, r.step,
                                                            r.kind == params::Kind::Int, params::kFloatDecimals);
                spinModel_->BeginDrag();
                SetCapture(Hwnd());
            }
            return 0;
        }
        // Anywhere else in the rollup: drag to scroll, if there is anything to scroll.
        if (rollup::MaxScroll(items) > 0 && y > rollup::kContainer.top && y < rollup::kContainer.bottom) {
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
            SpinnerModel m(RowValue(row), r.min, r.max, r.step, r.kind == params::Kind::Int, params::kFloatDecimals);
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
        if (LOWORD(lp) == HTCLIENT && ShowsRollouts() && rollup::MaxScroll(RolloutItems()) > 0) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(Hwnd(), &pt);
            const int x = pt.x * 96 / static_cast<int>(Dpi()), y = pt.y * 96 / static_cast<int>(Dpi());
            int row = -1;
            if (dragging_ || (y > rollup::kContainer.top && y < rollup::kContainer.bottom &&
                              rollup::TitleAt(RolloutItems(), scroll_, x, y) < 0 && TypeButtonAt(x, y) < 0 &&
                              ParamPartAt(x, y, row) == Part::None)) {
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            }
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wp) == kDropdownId && HIWORD(wp) == CBN_SELCHANGE && onSubcategoryChanged) {
            const LRESULT i = SendMessageW(dropdown_, CB_GETCURSEL, 0, 0);
            const auto items = Subcategories(category_);
            if (i >= 0 && static_cast<size_t>(i) < items.size()) onSubcategoryChanged(items[static_cast<size_t>(i)]);
            InvalidateRect(Hwnd(), nullptr, FALSE);   // the rollouts depend on the choice
        }
        return 0;
    case WM_SETFONT:
        font_ = reinterpret_cast<HFONT>(wp);
        if (dropdown_) SendMessageW(dropdown_, WM_SETFONT, wp, lp);   // the dropdown uses the UI font
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
