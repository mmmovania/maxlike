#include "win/App.h"
#include "ui/MainFrame.h"
#include <commctrl.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int /*cmdShow*/) {
    win::App::Init(instance);

    // Register the common-control window classes we use (tooltips).
    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_WIN95_CLASSES};
    InitCommonControlsEx(&icc);

    ui::MainFrame frame;
    if (!frame.Create()) {
        MessageBoxW(nullptr, L"Could not create the main window.", L"maxlike", MB_ICONERROR);
        return 1;
    }
    // 3ds Max R3 starts maximised.
    ShowWindow(frame.Hwnd(), SW_SHOWMAXIMIZED);
    UpdateWindow(frame.Hwnd());

    return win::App::Run();
}
