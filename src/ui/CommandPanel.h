#pragma once
#include "win/Window.h"
#include "ui/CommandPanelLayout.h"
#include "ui/Rollup.h"
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
    // A primitive button was checked (its index in StandardPrimitiveNames) or unchecked (-1).
    std::function<void(int type)> onCreateTypeChanged;

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
    bool ShowsRollouts() const;      // Create > Geometry > Standard Primitives
    void PaintRollouts(HDC dc);
    void PaintObjectType(HDC dc, int titleTop);
    void SetScroll(int scroll);
    int  TypeButtonAt(int x, int y) const;

    int selected_ = cmdpanel::kCreate;   // the reference shows the Create tab
    HBITMAP icons_ = nullptr;            // res/panel_tab_icons.bmp, 22 x 20 cells
    HBITMAP categoryIcons_ = nullptr;    // res/panel_category_icons.bmp, 16 x 16 cells
    int category_ = cmdpanel::kGeometry; // the reference shows Geometry pressed
    HWND dropdown_ = nullptr;            // classic COMBOBOX (draws R3's look itself)
    std::vector<rollup::Item> rollouts_{{cmdpanel::kObjectTypeContent, true}, {cmdpanel::kNameColorContent, true}};
    int scroll_ = 0;                     // pixels the rollup is scrolled up
    int createType_ = -1;                // checked primitive button, -1 = none
    bool dragging_ = false;              // scrolling the rollup by dragging (hand cursor)
    int dragStartY_ = 0, dragStartScroll_ = 0;
    HFONT font_ = nullptr;               // set by the parent with WM_SETFONT; not owned
};

} // namespace ui
