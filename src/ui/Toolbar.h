#pragma once
#include "win/Window.h"
#include "ui/FlyoutStrip.h"
#include "ui/ToolbarSpec.h"
#include <functional>
#include <string>
#include <vector>

namespace ui {

// A custom-drawn R3-style toolbar: buttons and separators drawn here, combo boxes and
// edit fields as real child windows, plus tooltips and flyouts. A toolbar::Config
// decides its items, layout and icons, so the same control serves as the main
// toolbar and as the button groups in the status area.
class Toolbar : public win::Window {
public:
    // Called with the command name when a button is clicked.
    std::function<void(const std::string&)> onCommand;
    // Called when the user picks an entry in a combo box (command name, entry text).
    std::function<void(const std::string&, const std::wstring&)> onComboChanged;
    // Called when the user presses Enter in an edit field (command name, text).
    std::function<void(const std::string&, const std::wstring&)> onEditCommitted;

    ~Toolbar() override;
    bool Create(HWND parent, toolbar::Config config);

    void SetEditText(const std::string& command, const std::wstring& text);

    // Which tab panel strip to show. 0 = Main Toolbar; the others are not built yet.
    void ShowStrip(int index, const std::wstring& label);

protected:
    const wchar_t* ClassName() const override { return L"maxlike.Toolbar"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void CreateControls();
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
    void PaintEditFrame(HDC dc, const Rect& r);
    void OnComboCommand(size_t index, int code);
    static LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM);

    toolbar::Config config_;
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
    std::vector<HWND> combos_;  // parallel to items_: the combo or edit child, else nullptr
    std::vector<WNDPROC> editOldProcs_; // parallel to items_: original EDIT procedures
    HWND tooltip_ = nullptr;    // owned popup; destroyed with this window
    HBITMAP icons_ = nullptr;
    HFONT font_ = nullptr;   // from the parent via WM_SETFONT; not owned
};

} // namespace ui
