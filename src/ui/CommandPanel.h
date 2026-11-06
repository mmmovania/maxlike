#pragma once
#include "win/Window.h"
#include "ui/CommandPanelLayout.h"
#include <functional>

namespace ui {

// The command panel on the right: six tabs (Create, Modify, Hierarchy, Motion,
// Display, Utilities) over a raised body that will hold the rollouts. Drawn to the
// measurements in docs/spec/01-ui-look.md; the outer frame is Edge::CommandPanel.
class CommandPanel : public win::Window {
public:
    std::function<void(int tab)> onTabChanged;
    std::function<void(int category)> onCategoryChanged;
    std::function<void(const std::wstring& subcategory)> onSubcategoryChanged;

    bool Create(HWND parent);
    int SelectedTab() const { return selected_; }
    void SelectTab(int tab);
    int SelectedCategory() const { return category_; }
    void SelectCategory(int category);

protected:
    const wchar_t* ClassName() const override { return L"maxlike.CommandPanel"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void OnPaint();
    void PaintTabs(HDC dc);
    void PaintCategories(HDC dc);
    void FillDropdown();
    void UpdateCreateControls();     // the dropdown shows on the Create tab only

    int selected_ = cmdpanel::kCreate;   // the reference shows the Create tab
    HBITMAP icons_ = nullptr;            // res/panel_tab_icons.bmp, 22 x 20 cells
    HBITMAP categoryIcons_ = nullptr;    // res/panel_category_icons.bmp, 16 x 16 cells
    int category_ = cmdpanel::kGeometry; // the reference shows Geometry pressed
    HWND dropdown_ = nullptr;            // classic COMBOBOX (draws R3's look itself)
    HFONT font_ = nullptr;               // set by the parent with WM_SETFONT; not owned
};

} // namespace ui
