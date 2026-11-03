#pragma once
#include "win/Window.h"
#include "ui/ToolbarSpec.h"
#include <vector>

namespace ui {

// The popup strip of alternatives that opens when a flyout button is held down.
// It never takes the mouse itself: the toolbar keeps mouse capture during the
// press and tells the strip which cell is under the mouse (SetHot).
class FlyoutStrip : public win::Window {
public:
    // Opens the strip with its first cell exactly over `buttonScreen`.
    // `iconSize` and the offsets are at 100% (as in the toolbar's Config).
    bool Open(HWND owner, const std::vector<toolbar::Alternative>& alternatives,
              int current, HBITMAP icons, const RECT& buttonScreen,
              int iconSize, int iconOffsetX, int iconOffsetY);
    void Close();
    bool IsOpen() const { return Hwnd() != nullptr; }

    int  CellAt(POINT screen) const; // cell index under a screen point, or -1
    void SetHot(int cell);
    int  Hot() const { return hot_; }

protected:
    const wchar_t* ClassName() const override { return L"maxlike.FlyoutStrip"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    RECT CellRect(int cell) const;   // client coordinates
    void OnPaint();

    std::vector<toolbar::Alternative> alternatives_;
    int current_ = 0;
    int hot_ = -1;
    int cellW_ = 0, cellH_ = 0, border_ = 0;
    HBITMAP icons_ = nullptr; // owned by the toolbar
    int iconSize_ = toolbar::kIconSize, iconOffsetX_ = toolbar::kIconOffsetX, iconOffsetY_ = toolbar::kIconOffsetY;
};

} // namespace ui
