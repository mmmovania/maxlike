#include "ui/Placeholder.h"
#include "ui/Theme.h"
#include <cstdio>

namespace ui {

Placeholder::Placeholder(std::wstring label, Edge edge, COLORREF fill, COLORREF text)
    : label_(std::move(label)), edge_(edge), fill_(fill), text_(text) {}

bool Placeholder::Create(HWND parent) {
    CreateParams p;
    p.title  = label_.c_str();
    p.style  = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    p.x = p.y = 0;
    p.width = p.height = 0;     // the parent's layout positions us
    p.parent = parent;
    return win::Window::Create(p);
}

void Placeholder::SetLabel(std::wstring label) {
    label_ = std::move(label);
    if (Hwnd()) InvalidateRect(Hwnd(), nullptr, FALSE);
}

void Placeholder::OnPaint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(Hwnd(), &ps);

    RECT rc;
    GetClientRect(Hwnd(), &rc);
    const COLORREF fill = (fill_ == kSystemFace) ? theme::Face() : fill_;
    FillSolid(dc, rc, fill);
    const RECT inner = DrawFrameEdge(dc, rc, edge_);

    wchar_t text[128];
    swprintf_s(text, L"%s  %d \u00D7 %d", label_.c_str(),
               static_cast<int>(rc.right - rc.left), static_cast<int>(rc.bottom - rc.top));

    HGDIOBJ oldFont = SelectObject(dc, font_ ? font_ : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, (text_ == kSystemFace) ? GetSysColor(COLOR_GRAYTEXT) : text_);
    RECT textRc = inner;
    DrawTextW(dc, text, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, oldFont);

    EndPaint(Hwnd(), &ps);
}

LRESULT Placeholder::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        OnPaint();
        return 0;
    case WM_SETFONT:                       // standard Win32 convention: parent hands
        font_ = reinterpret_cast<HFONT>(wp); // children the font to use
        if (LOWORD(lp)) InvalidateRect(Hwnd(), nullptr, FALSE);
        return 0;
    case WM_GETFONT:
        return reinterpret_cast<LRESULT>(font_);
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
