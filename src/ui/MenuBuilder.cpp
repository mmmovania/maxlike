#include "ui/MenuBuilder.h"

namespace ui {

static void AppendItems(HMENU menu, const std::vector<MenuItem>& items, CommandTable& commands) {
    for (const MenuItem& it : items) {
        if (it.IsSeparator()) {
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        } else if (it.IsSubmenu()) {
            HMENU sub = CreatePopupMenu();
            AppendItems(sub, it.children, commands);
            // MF_POPUP: the "ID" argument is the submenu handle; the parent now owns it.
            AppendMenuW(menu, MF_POPUP | MF_STRING, reinterpret_cast<UINT_PTR>(sub), it.text.c_str());
        } else {
            const UINT_PTR id = commands.Register(it.command);
            AppendMenuW(menu, MF_STRING, id, it.text.c_str());
        }
    }
}

HMENU BuildMenuBar(const std::vector<MenuItem>& spec, CommandTable& commands) {
    HMENU bar = CreateMenu(); // a menu *bar*; dropdowns are CreatePopupMenu
    AppendItems(bar, spec, commands);
    return bar;
}

} // namespace ui
