#pragma once
#include <windows.h>

namespace win {

class App {
public:
    static void Init(HINSTANCE instance) { instance_ = instance; }
    static HINSTANCE Instance() { return instance_; }

    // Standard message loop; returns the WM_QUIT exit code.
    static int Run();

private:
    static inline HINSTANCE instance_ = nullptr;
};

} // namespace win
