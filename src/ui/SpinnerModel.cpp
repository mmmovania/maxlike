#include "ui/SpinnerModel.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <cwctype>

namespace ui {

SpinnerModel::SpinnerModel(double value, double min, double max, double step, bool integer, int decimals)
    : value_(0), min_(std::min(min, max)), max_(std::max(min, max)), step_(step), integer_(integer),
      decimals_(integer ? 0 : decimals) {
    Set(value);
}

void SpinnerModel::Set(double v) {
    if (std::isnan(v)) return;
    if (integer_) v = std::round(v);
    value_ = std::clamp(v, min_, max_);
}

void SpinnerModel::StepBy(int clicks) { Set(value_ + clicks * step_); }

void SpinnerModel::Reset() { Set(0.0); }   // clamping puts it at the nearest limit if 0 is out of range

void SpinnerModel::BeginDrag() {
    dragging_ = true;
    dragStart_ = value_;
}

void SpinnerModel::DragTo(int pixelsUp, bool ctrl, bool alt) {
    if (!dragging_) return;
    double perPixel = step_;
    if (ctrl) perPixel *= 10;
    if (alt) perPixel /= 10;
    Set(dragStart_ + pixelsUp * perPixel);
}

void SpinnerModel::EndDrag() { dragging_ = false; }

void SpinnerModel::CancelDrag() {
    if (!dragging_) return;
    dragging_ = false;
    value_ = dragStart_;
}

std::wstring SpinnerModel::Text() const {
    // Round halves away from zero first: printf rounds an exact half to the even digit
    // (3.25 -> "3.2"), which is not what people expect to read.
    const double scale = std::pow(10.0, decimals_);
    const double shown = std::round(value_ * scale) / scale;
    wchar_t buf[64];
    std::swprintf(buf, 64, L"%.*f", decimals_, shown);
    // "-0.0" reads oddly; show it as "0.0".
    if (buf[0] == L'-' && std::wcstod(buf, nullptr) == 0.0) return std::wstring(buf + 1);
    return buf;
}

bool SpinnerModel::SetText(const std::wstring& text) {
    const wchar_t* s = text.c_str();
    while (*s && std::iswspace(*s)) ++s;
    if (!*s) return false;
    wchar_t* end = nullptr;
    const double v = std::wcstod(s, &end);
    if (end == s) return false;
    while (*end && std::iswspace(*end)) ++end;
    if (*end) return false;                    // trailing junk: reject the whole entry
    Set(v);
    return true;
}

} // namespace ui
