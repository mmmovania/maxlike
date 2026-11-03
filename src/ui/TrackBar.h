#pragma once
#include "win/Window.h"

namespace ui {

// The R3 track bar under the time slider. For now it draws its frame and a marker
// at the current frame; keys and frame ticks come with animation (later phase).
class TrackBar : public win::Window {
public:
    bool Create(HWND parent);
    void SetRange(int start, int end);
    void SetFrame(int frame);

protected:
    const wchar_t* ClassName() const override { return L"maxlike.TrackBar"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void OnPaint();
    int start_ = 0, end_ = 100, frame_ = 0;
};

} // namespace ui
