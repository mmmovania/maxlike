#include "win/App.h"

namespace win {

int App::Run() {
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (modeless_ && IsWindow(modeless_) && IsDialogMessageW(modeless_, &msg)) continue;
        // Shortcuts only for messages of the main window or its children (not dialogs).
        if (target_ && (msg.hwnd == target_ || IsChild(target_, msg.hwnd))) {
            if (always_ && TranslateAcceleratorW(target_, always_, &msg)) continue;
            if (plain_ && !FocusIsTextField() && TranslateAcceleratorW(target_, plain_, &msg)) continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

bool App::FocusIsTextField() {
    wchar_t cls[32] = L"";
    const HWND focus = GetFocus();
    return focus && GetClassNameW(focus, cls, 32) && _wcsicmp(cls, L"Edit") == 0;
}

} // namespace win
