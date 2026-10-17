#include "win/App.h"
#include "ui/MainFrame.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int /*cmdShow*/) {
    win::App::Init(instance);

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
