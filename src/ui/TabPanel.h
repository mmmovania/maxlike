#pragma once
#include "win/Window.h"
#include <functional>

namespace ui {

// The R3 tab panel: a row of fixed-width tabs above the main toolbar.
// Custom-drawn to match the reference pixel for pixel (see TabPanel.cpp).
class TabPanel : public win::Window {
public:
    // Called with the new index when the user picks a different tab.
    std::function<void(int)> onTabChanged;

    bool Create(HWND parent);

    int  ActiveTab() const { return active_; }
    void SetActiveTab(int index); // does not call onTabChanged

protected:
    const wchar_t* ClassName() const override { return L"maxlike.TabPanel"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void OnPaint();
    void PaintTab(HDC dc, int index, int height);

    int   active_ = 0;
    HFONT font_   = nullptr; // from the parent via WM_SETFONT; not owned
};

} // namespace ui
