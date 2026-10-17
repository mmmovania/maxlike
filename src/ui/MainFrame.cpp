#include "ui/MainFrame.h"
#include "ui/Theme.h"
#include <cstdio>

namespace ui {

static constexpr const wchar_t* kAppName = L"maxlike";

MainFrame::~MainFrame() {
    if (uiFont_) DeleteObject(uiFont_);
}

bool MainFrame::Create() {
    std::wstring title = docName_ + L" - " + kAppName;
    CreateParams p;
    p.title  = title.c_str();
    p.style  = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    p.width  = 1280;
    p.height = 800;
    return win::Window::Create(p);
}

void MainFrame::SetDocumentName(const std::wstring& name) {
    docName_ = name;
    if (Hwnd()) {
        std::wstring title = docName_ + L" - " + kAppName;
        SetWindowTextW(Hwnd(), title.c_str());
    }
}

void MainFrame::RecreateFonts() {
    if (uiFont_) DeleteObject(uiFont_);
    uiFont_ = theme::CreateUiFont(Dpi());
}

void MainFrame::OnPaint() {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(Hwnd(), &ps);
    FillRect(dc, &ps.rcPaint, theme::FaceBrush());

    // Temporary Phase 0 diagnostic: proves the font and DPI handling.
    // Removed in Phase 1 when real layout regions appear.
    wchar_t text[96];
    swprintf_s(text, L"maxlike %hs  |  DPI %u (%u%%)",
               MAXLIKE_VERSION, Dpi(), Dpi() * 100 / 96);
    HGDIOBJ old = SelectObject(dc, uiFont_);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
    TextOutW(dc, Scale(8), Scale(8), text, static_cast<int>(wcslen(text)));
    SelectObject(dc, old);

    EndPaint(Hwnd(), &ps);
}

LRESULT MainFrame::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        RecreateFonts();
        return 0;

    case WM_ERASEBKGND:
        return 1; // painted in WM_PAINT; avoids flicker

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_GETMINMAXINFO: {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
        mmi->ptMinTrackSize.x = Scale(theme::Metrics::MinWindowWidth);
        mmi->ptMinTrackSize.y = Scale(theme::Metrics::MinWindowHeight);
        return 0;
    }

    case WM_DPICHANGED: {
        const RECT* r = reinterpret_cast<const RECT*>(lp);
        SetWindowPos(Hwnd(), nullptr, r->left, r->top,
                     r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        RecreateFonts();
        InvalidateRect(Hwnd(), nullptr, FALSE);
        return 0;
    }

    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
        InvalidateRect(Hwnd(), nullptr, FALSE);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
