#include "win/App.h"
#include "ui/MainFrame.h"
#include <commctrl.h>
#include <cwchar>
#include <string>

// Development option: `--client-size WxH` opens the window un-maximised with an
// exact client area (below the menu bar), e.g. 1920x998 like the R3.1 reference
// screenshot. Used for pixel comparisons; returns false if the option is absent.
static bool ParseClientSize(const wchar_t* cmdLine, int& w, int& h) {
    const wchar_t* p = cmdLine ? std::wcsstr(cmdLine, L"--client-size ") : nullptr;
    return p && std::swscanf(p + 14, L"%dx%d", &w, &h) == 2 && w > 0 && h > 0;
}

// Development option: `--demo-scene box|plane|pyramid|all` opens with objects from the
// verified sample files, to compare with R3.1.
static std::string DemoSceneArg(const wchar_t* cmdLine) {
    const wchar_t* p = cmdLine ? std::wcsstr(cmdLine, L"--demo-scene ") : nullptr;
    if (!p) return {};
    p += 13;
    std::string name;
    while (*p && *p != L' ') name += static_cast<char>(*p++);
    return name;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR cmdLine, int /*cmdShow*/) {
    win::App::Init(instance);

    // Register the common-control window classes we use (tooltips).
    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_WIN95_CLASSES};
    InitCommonControlsEx(&icc);

    ui::MainFrame frame;
    if (!frame.Create()) {
        MessageBoxW(nullptr, L"Could not create the main window.", L"maxlike", MB_ICONERROR);
        return 1;
    }
    const std::string demo = DemoSceneArg(cmdLine);
    if (!demo.empty() && !frame.LoadDemoScene(demo))
        MessageBoxW(frame.Hwnd(), L"Unknown demo scene. Use box, plane, pyramid or all.", L"maxlike", MB_ICONWARNING);

    int clientW = 0, clientH = 0;
    if (ParseClientSize(cmdLine, clientW, clientH)) {
        // Grow the window until its client area (excluding title and menu bars)
        // has the requested size; the menu can wrap, so measure and correct.
        SetWindowPos(frame.Hwnd(), nullptr, 0, 0, clientW, clientH, SWP_NOZORDER);
        for (int i = 0; i < 3; ++i) {
            RECT wr, cr;
            GetWindowRect(frame.Hwnd(), &wr);
            GetClientRect(frame.Hwnd(), &cr);
            SetWindowPos(frame.Hwnd(), nullptr, 0, 0,
                         (wr.right - wr.left) + clientW - cr.right,
                         (wr.bottom - wr.top) + clientH - cr.bottom, SWP_NOZORDER);
        }
        ShowWindow(frame.Hwnd(), SW_SHOWNORMAL);
    } else {
        // 3ds Max R3 starts maximised.
        ShowWindow(frame.Hwnd(), SW_SHOWMAXIMIZED);
    }
    UpdateWindow(frame.Hwnd());

    return win::App::Run();
}
