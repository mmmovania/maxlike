#pragma once
#include "win/Window.h"
#include "ui/StatusLayout.h"
#include <functional>
#include <string>

namespace ui {

// The left part of the R3 status area (step 1.5b): MAXScript mini-listener (two real
// EDIT controls), a draggable splitter, two prompt lines, the selection-lock toggle,
// X/Y/Z fields and the grid-spacing field. The right part follows in step 1.5c.
class StatusBar : public win::Window {
public:
    // The user pressed Enter in the listener's input line.
    std::function<void(const std::wstring&)> onListenerInput;
    // The user toggled the selection lock.
    std::function<void(bool)> onLockChanged;

    ~StatusBar() override;
    bool Create(HWND parent);

    void SetPrompt(int line, const std::wstring& text); // line 0 = top, 1 = bottom
    void SetListenerOutput(const std::wstring& text);   // the pink line
    void SetCoordinates(const std::wstring& x, const std::wstring& y, const std::wstring& z);
    void SetGridSpacing(double spacing);

protected:
    const wchar_t* ClassName() const override { return L"maxlike.StatusBar"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void Relayout();
    void OnPaint();
    void PaintField(HDC dc, const Rect& r, const std::wstring& text);
    void PaintLock(HDC dc, const Rect& r);
    static LRESULT CALLBACK InputProc(HWND, UINT, WPARAM, LPARAM);

    statusbar::Layout layout_{};
    int listenerWidth_ = statusbar::kDefaultListenerWidth; // at 100%
    bool draggingSplitter_ = false;
    int  dragStartX_ = 0, dragStartWidth_ = 0;
    bool locked_ = false;
    bool lockPressed_ = false;

    std::wstring prompt_[2] = {L"None Selected", L"Click or click-and-drag to select objects"};
    std::wstring x_, y_, z_;
    std::wstring grid_ = L"Grid = 10.0";

    HWND output_ = nullptr;          // pink, read-only
    HWND input_  = nullptr;          // white, editable
    WNDPROC inputOldProc_ = nullptr; // for the Enter-key subclass
    HBRUSH outputBrush_ = nullptr;
    HFONT font_ = nullptr;           // from the parent; not owned
};

} // namespace ui
