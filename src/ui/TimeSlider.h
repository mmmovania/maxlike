#pragma once
#include "win/Window.h"
#include "ui/TimeSliderGeometry.h"
#include <functional>

namespace ui {

// The R3 time slider: a "<" button, a "frame / end" thumb and a ">" button that
// slide together along a trough. Drag the thumb to scrub; the arrows step one
// frame (auto-repeating while held); clicking the trough jumps there.
class TimeSlider : public win::Window {
public:
    std::function<void(int)> onFrameChanged; // user changed the frame

    bool Create(HWND parent);
    void SetRange(int start, int end);
    void SetFrame(int frame);                // does not call onFrameChanged
    int  Frame() const { return frame_; }

protected:
    const wchar_t* ClassName() const override { return L"maxlike.TimeSlider"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    enum class Part { None, Prev, Thumb, Next, Trough };

    timeslider::Parts CurrentParts() const;
    Part HitTest(int x, int y) const;
    void ChangeFrame(int frame);             // set + notify
    void OnPaint();
    void PaintArrow(HDC dc, const Rect& r, bool left, bool down);
    void PaintThumb(HDC dc, const Rect& r);

    int start_ = 0, end_ = 100, frame_ = 0;
    Part pressed_ = Part::None;
    int  dragOffset_ = 0;   // mouse x minus block left, while dragging the thumb
    HFONT font_ = nullptr;  // from the parent; not owned
};

} // namespace ui
