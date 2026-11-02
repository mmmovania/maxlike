#pragma once
#include "win/Window.h"
#include "ui/ToolbarSpec.h"
#include <functional>
#include <string>
#include <vector>

namespace ui {

// The R3 main toolbar: one custom-drawn window containing all buttons and
// separators (combo boxes become real child controls in step 1.4b).
class Toolbar : public win::Window {
public:
    // Called with the command name when a button is clicked.
    std::function<void(const std::string&)> onCommand;

    ~Toolbar() override;
    bool Create(HWND parent);

    // Which tab panel strip to show. 0 = Main Toolbar; the others are not built yet.
    void ShowStrip(int index, const std::wstring& label);

protected:
    const wchar_t* ClassName() const override { return L"maxlike.Toolbar"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void Relayout();
    int  HitTest(int x, int y) const;        // index of a button, or -1
    void Activate(size_t index);             // a completed click
    void InvalidateItem(int index);

    void OnPaint();
    void PaintButton(HDC dc, HDC iconDc, size_t index);
    void PaintSeparator(HDC dc, const Rect& r);
    void PaintCombo(HDC dc, size_t index);

    std::vector<toolbar::Item> items_;
    std::vector<Rect> rects_;
    std::vector<bool> checked_;
    int  hot_ = -1;          // button under the mouse
    int  pressed_ = -1;      // button the mouse went down on
    bool trackingLeave_ = false;
    int  strip_ = 0;
    std::wstring stripLabel_;
    HBITMAP icons_ = nullptr;
    HFONT font_ = nullptr;   // from the parent via WM_SETFONT; not owned
};

} // namespace ui
