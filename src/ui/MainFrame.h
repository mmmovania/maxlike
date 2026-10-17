#pragma once
#include "win/Window.h"
#include <string>

namespace ui {

// Top-level application window. Phase 0: empty face-coloured client area.
class MainFrame : public win::Window {
public:
    ~MainFrame() override;

    bool Create();
    void SetDocumentName(const std::wstring& name); // updates "<name> - maxlike"

protected:
    const wchar_t* ClassName() const override { return L"maxlike.MainFrame"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void RecreateFonts();
    void OnPaint();

    HFONT uiFont_ = nullptr;
    std::wstring docName_ = L"Untitled";
};

} // namespace ui
