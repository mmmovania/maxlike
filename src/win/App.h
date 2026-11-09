#pragma once
#include <windows.h>

namespace win {

class App {
public:
    static void Init(HINSTANCE instance) { instance_ = instance; }
    static HINSTANCE Instance() { return instance_; }

    // Standard message loop; returns the WM_QUIT exit code.
    static int Run();

    // Keyboard shortcuts for `target`: `always` (Ctrl / Alt / function keys) work
    // everywhere; `plain` (single keys, Shift+key) not while a text field has focus, so
    // typing a name or a value is never taken for a shortcut.
    static void SetAccelerators(HWND target, HACCEL always, HACCEL plain) { target_ = target; always_ = always; plain_ = plain; }
    // A modeless dialog (the Transform Type-In) gets Tab, Enter and Escape (null: none).
    static void SetModelessDialog(HWND dialog) { modeless_ = dialog; }

private:
    static bool FocusIsTextField();
    static inline HINSTANCE instance_ = nullptr;
    static inline HWND target_ = nullptr;
    static inline HACCEL always_ = nullptr, plain_ = nullptr;
    static inline HWND modeless_ = nullptr;
};

} // namespace win
