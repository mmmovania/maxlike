#pragma once
#include "win/Window.h"
#include "ui/FlyoutStrip.h"
#include "ui/ToolbarSpec.h"
#include <functional>
#include <string>
#include <vector>

namespace ui {

// The R3 main toolbar: one custom-drawn window for the buttons and separators,
// with real combo-box child windows and a tooltip control.
class Toolbar : public win::Window {
public:
    // Called with the command name when a button is clicked.
    std::function<void(const std::string&)> onCommand;
    // Called when the user picks an entry in a combo box (command name, entry text).
    std::function<void(const std::string&, const std::wstring&)> onComboChanged;

    ~Toolbar() override;
    bool Create(HWND parent);

    // Which tab panel strip to show. 0 = Main Toolbar; the others are not built yet.
    void ShowStrip(int index, const std::wstring& label);

protected:
    const wchar_t* ClassName() const override { return L"maxlike.Toolbar"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void CreateCombos();
    void CreateTooltips();
    void Relayout();
    int  HitTest(int x, int y) const;        // index of a button, or -1
    void Activate(size_t index);             // a completed click
    void OpenFlyout();                       // the held button's strip
    void CloseFlyout();
    void ChooseAlternative(size_t index, int alternative);
    void InvalidateItem(int index);

    void OnPaint();
    void PaintButton(HDC dc, HDC iconDc, size_t index);
    void PaintSeparator(HDC dc, const Rect& r);
    void OnComboCommand(size_t index, int code);

    std::vector<toolbar::Item> items_;
    std::vector<Rect> rects_;
    std::vector<bool> checked_;
    FlyoutStrip flyout_;
    int  flyoutItem_ = -1;   // item whose strip is open
    int  hot_ = -1;          // button under the mouse
    int  pressed_ = -1;      // button the mouse went down on
    bool trackingLeave_ = false;
    int  strip_ = 0;
    std::wstring stripLabel_;
    std::vector<HWND> combos_;  // parallel to items_; nullptr for non-combo items
    HWND tooltip_ = nullptr;    // owned popup; destroyed with this window
    HBITMAP icons_ = nullptr;
    HFONT font_ = nullptr;   // from the parent via WM_SETFONT; not owned
};

} // namespace ui
