#pragma once
#include "win/Window.h"
#include "ui/Bevel.h"
#include <string>

namespace ui {

// Temporary stand-in for a main-window region. Paints a fill, an edge and its
// name plus current size, so the layout can be checked against the reference
// screenshot. Each one is replaced by a real control later in Phase 1/2.
class Placeholder : public win::Window {
public:
    static constexpr COLORREF kSystemFace = 0xFFFFFFFF; // "use COLOR_BTNFACE"

    Placeholder(std::wstring label, Edge edge,
                COLORREF fill = kSystemFace, COLORREF text = kSystemFace);

    bool Create(HWND parent);

protected:
    const wchar_t* ClassName() const override { return L"maxlike.Placeholder"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void OnPaint();

    std::wstring label_;
    Edge edge_;
    COLORREF fill_;
    COLORREF text_;
    HFONT font_ = nullptr; // set by the parent with WM_SETFONT; not owned
};

} // namespace ui
