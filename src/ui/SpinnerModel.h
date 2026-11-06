#pragma once
#include <string>

// A spinner's behaviour (value, limits, stepping, dragging, reset, typed entry),
// separate from how it is drawn. No Windows dependency (unit-tested in
// tests/test_spinner.cpp). The look is measured from an R3.1 screenshot (step 5.3b).
//
// Conventions of R3's spinners, from memory (NOTE: to be verified against R3.1):
//   - clicking an arrow steps the value by `step`;
//   - dragging on the arrows changes it by `step` per pixel of vertical movement (up =
//     more); Ctrl makes it 10x faster, Alt 10x slower;
//   - right-click resets it to 0, or to the nearest limit if 0 is out of range;
//     right-click during a drag cancels the drag;
//   - typed text is parsed as a number; anything else leaves the value unchanged.
namespace ui {

class SpinnerModel {
public:
    SpinnerModel(double value, double min, double max, double step, bool integer = false, int decimals = 1);

    double Value() const { return value_; }
    void   Set(double v);                     // clamped to [min, max], rounded if integer

    void   StepBy(int clicks);                // arrow clicks (+1 up, -1 down)
    void   Reset();                           // right-click

    // Dragging: one change, from BeginDrag to EndDrag (one undo step later on).
    void   BeginDrag();
    void   DragTo(int pixelsUp, bool ctrl, bool alt);   // total movement since BeginDrag
    void   EndDrag();                         // keep the dragged value
    void   CancelDrag();                      // right-click while dragging: back to the start
    bool   Dragging() const { return dragging_; }

    std::wstring Text() const;                // as shown in the field
    bool   SetText(const std::wstring& text); // typed entry; false (no change) if invalid

    double Min() const { return min_; }
    double Max() const { return max_; }

private:
    double value_, min_, max_, step_;
    bool integer_;
    int decimals_;
    bool dragging_ = false;
    double dragStart_ = 0;
};

} // namespace ui
