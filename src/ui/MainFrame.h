#pragma once
#include "win/Window.h"
#include "ui/Placeholder.h"
#include <array>
#include <string>

namespace ui {

// Top-level application window. Owns the main regions and lays them out.
class MainFrame : public win::Window {
public:
    ~MainFrame() override;

    bool Create();
    void SetDocumentName(const std::wstring& name); // updates "<name> - maxlike"

protected:
    const wchar_t* ClassName() const override { return L"maxlike.MainFrame"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    bool CreateRegions();
    void LayoutRegions(int clientWidth, int clientHeight);
    void RecreateFonts();
    void OnPaint();

    std::array<Placeholder*, 7> Regions();

    HFONT uiFont_ = nullptr;
    std::wstring docName_ = L"Untitled";

    // Regions, in the order of FrameLayout. Placeholders for now; each is
    // replaced by its real control in later steps.
    Placeholder tabPanel_    {L"Tab panel",     Edge::Outline};
    Placeholder mainToolbar_ {L"Main toolbar",  Edge::Outline};
    Placeholder viewports_   {L"Viewports",     Edge::ViewportFrame,
                              RGB(0xB0, 0xB0, 0xB0), RGB(0xE5, 0xE5, 0xE5)};
    Placeholder timeSlider_  {L"Time slider",   Edge::Outline};
    Placeholder trackBar_    {L"Track bar",     Edge::Sunken};
    Placeholder commandPanel_{L"Command panel", Edge::Raised};
    Placeholder statusArea_  {L"Status area",   Edge::Outline};
};

} // namespace ui
