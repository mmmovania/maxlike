// Unit tests for the menu specification and the command table.
#include "ui/CommandTable.h"
#include "ui/MenuSpec.h"
#include "ui/RecentFiles.h"
#include <algorithm>
#include <functional>
#include <cstdio>
#include <cwctype>
#include <set>
#include <string>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static void TopLevelMatchesReference() {
    // Titles as they appear in the R3.1 reference screenshot, left to right.
    const wchar_t* expected[] = {L"File", L"Edit", L"Tools", L"Group", L"Views", L"Rendering",
                                 L"Track View", L"Schematic View", L"Customize", L"MAXScript", L"Help"};
    const auto& spec = ui::MainMenuSpec();
    CHECK(spec.size() == std::size(expected));
    for (size_t i = 0; i < spec.size() && i < std::size(expected); ++i) {
        CHECK(ui::PlainMenuText(spec[i].text) == expected[i]);
        CHECK(spec[i].IsSubmenu());
    }
}

// Walks every menu: commands unique and well-formed; separators never first,
// last or doubled; mnemonic letters unique within one menu.
static void CheckMenu(const std::vector<ui::MenuItem>& items, std::set<std::string>& seen) {
    std::set<wchar_t> mnemonics;
    for (size_t i = 0; i < items.size(); ++i) {
        const ui::MenuItem& it = items[i];
        if (it.IsSeparator()) {
            CHECK(i != 0 && i + 1 != items.size());
            CHECK(!items[i - 1].IsSeparator());
            continue;
        }
        CHECK(it.IsSubmenu() != !it.command.empty()); // exactly one of the two
        if (!it.command.empty()) {
            CHECK(seen.insert(it.command).second);         // unique
            CHECK(it.command.find('.') != std::string::npos); // "menu.item"
        }
        if (auto amp = it.text.find(L'&'); amp != std::wstring::npos && amp + 1 < it.text.size()) {
            const wchar_t m = static_cast<wchar_t>(std::towlower(it.text[amp + 1]));
            CHECK(mnemonics.insert(m).second);
        }
        if (it.IsSubmenu()) CheckMenu(it.children, seen);
    }
}

static void MenusAreWellFormed() {
    std::set<std::string> seen;
    CheckMenu(ui::MainMenuSpec(), seen);
    CHECK(seen.count("file.exit") == 1);
    CHECK(seen.count("help.about") == 1);
}

static void CommandTableRoundTrips() {
    ui::CommandTable t;
    const auto a = t.Register("file.open");
    const auto b = t.Register("file.save");
    CHECK(a == ui::CommandTable::kFirstId);
    CHECK(b == a + 1);
    CHECK(t.Register("file.open") == a);  // registering again returns the same ID
    CHECK(t.Find("file.save") == b);
    CHECK(t.Find("nope") == 0);
    CHECK(t.Name(a) == "file.open");
    CHECK(t.Name(12345).empty());
}

static void PlainTextStripsMarkers() {
    CHECK(ui::PlainMenuText(L"&Open...\tCtrl+O") == L"Open...");
    CHECK(ui::PlainMenuText(L"Trac&k View") == L"Track View");
    CHECK(ui::PlainMenuText(L"Save && Exit") == L"Save & Exit");
}

int main() {
    TopLevelMatchesReference();
    MenusAreWellFormed();
    CommandTableRoundTrips();
    PlainTextStripsMarkers();
    // The shortcut column, from R3.1's menus opened (r3.1-menu-*.png): every shortcut
    // parses, each to the key the screenshots show, and no key is used twice.
    {
        std::vector<std::wstring> unparsed;
        const auto shortcuts = ui::ShortcutsOf(ui::MainMenuSpec(), &unparsed);
        CHECK(unparsed.empty());
        struct E { const char* command; uint16_t key; bool ctrl, alt, shift; };
        const E expected[] = {
            {"file.new", 'N', true, false, false}, {"file.open", 'O', true, false, false}, {"file.save", 'S', true, false, false},
            {"edit.undo", 'Z', true, false, false}, {"edit.redo", 'A', true, false, false},
            {"edit.hold", 'H', true, true, false}, {"edit.fetch", 'F', true, true, false}, {"edit.delete", 0x2E, false, false, false},
            {"tools.transform_type_in", 0x7B, false, false, false}, {"tools.align", 'A', false, true, false},
            {"tools.align_normals", 'N', false, true, false}, {"tools.place_highlight", 'H', true, false, false},
            {"tools.material_editor", 'M', false, false, false}, {"tools.spacing", 'I', false, false, true},
            {"views.undo", 'Z', false, false, true}, {"views.redo", 'A', false, false, true},
            {"views.background", 'B', false, true, false}, {"views.update_background", 'B', true, true, true},
            {"views.match_camera", 'C', true, false, false}, {"views.redraw_all", '1', false, false, false},
            {"views.expert_mode", 'X', true, false, false}, {"render.show_last", 'I', true, false, false},
            {"maxscript.listener", 0x7A, false, false, false}};
        CHECK(shortcuts.size() == sizeof(expected) / sizeof(expected[0]));
        for (const E& e : expected) {
            bool found = false;
            for (const auto& s : shortcuts)
                found = found || (s.command == e.command && s.key == e.key && s.ctrl == e.ctrl && s.alt == e.alt && s.shift == e.shift);
            CHECK(found);
            if (!found) std::printf("  missing shortcut for %s\n", e.command);
        }
        for (size_t i = 0; i < shortcuts.size(); ++i)
            for (size_t j = i + 1; j < shortcuts.size(); ++j)
                CHECK(!(shortcuts[i].key == shortcuts[j].key && shortcuts[i].ctrl == shortcuts[j].ctrl &&
                        shortcuts[i].alt == shortcuts[j].alt && shortcuts[i].shift == shortcuts[j].shift));
        // Plain and Shift-only keys must not interrupt typing; Ctrl/Alt and F-keys may.
        for (const auto& s : shortcuts) {
            if (s.command == "tools.material_editor" || s.command == "views.redraw_all" || s.command == "edit.delete" || s.command == "views.undo") CHECK(!s.WhileTyping());
            if (s.command == "file.save" || s.command == "tools.transform_type_in" || s.command == "edit.hold") CHECK(s.WhileTyping());
        }
    }
    // Greying, as in R3.1's screenshots of an empty scene: exactly these commands greyed.
    {
        const std::set<std::string> greyed = {
            "file.save_selected", "file.merge_animation", "file.export_selected",
            "edit.undo", "edit.redo", "edit.clone", "edit.select_all", "edit.select_none", "edit.select_invert", "edit.properties",
            "group.group", "group.open", "group.close", "group.ungroup", "group.explode", "group.detach", "group.attach",
            "views.undo", "views.redo", "views.restore_active", "views.update_background", "views.reset_background_transform",
            "views.match_camera", "views.add_default_lights", "render.show_last", "trackview.delete", "schematic.delete"};
        std::vector<std::string> all;
        std::function<void(const std::vector<ui::MenuItem>&)> walk = [&](const std::vector<ui::MenuItem>& items) {
            for (const auto& it : items) { if (it.IsSubmenu()) walk(it.children); else if (!it.command.empty()) all.push_back(it.command); }
        };
        walk(ui::MainMenuSpec());
        const ui::MenuState empty;
        for (const auto& c : all) {
            const bool expectEnabled = greyed.count(c) == 0;
            CHECK(ui::MenuItemEnabled(c, empty) == expectEnabled);
            if (ui::MenuItemEnabled(c, empty) != expectEnabled) std::printf("  %s: %s in maxlike, %s in R3.1\n", c.c_str(), expectEnabled ? "greyed" : "enabled", expectEnabled ? "enabled" : "greyed");
        }
        for (const auto& g : greyed) CHECK(std::find(all.begin(), all.end(), g) != all.end());   // every greyed item exists
        // With objects and a selection, the selection items come alive; groups stay greyed.
        ui::MenuState busy;
        busy.objects = 3; busy.selected = 1; busy.canUndo = true;
        for (const char* c : {"edit.clone", "edit.select_none", "edit.properties", "file.save_selected", "group.group", "edit.select_all", "edit.undo"})
            CHECK(ui::MenuItemEnabled(c, busy));
        CHECK(!ui::MenuItemEnabled("group.ungroup", busy) && !ui::MenuItemEnabled("edit.redo", busy));
    }
    // Recent files: newest first, one entry per file (case-insensitive), nine at most.
    {
        std::vector<std::wstring> r;
        for (int i = 1; i <= 12; ++i) r = ui::AddRecentFile(r, L"C:\\s\\f" + std::to_wstring(i) + L".max");
        CHECK(r.size() == 9 && r[0] == L"C:\\s\\f12.max" && r[8] == L"C:\\s\\f4.max");
        r = ui::AddRecentFile(r, L"c:\\S\\F7.MAX");
        CHECK(r.size() == 9 && r[0] == L"c:\\S\\F7.MAX" && r[1] == L"C:\\s\\f12.max");
        for (size_t i = 1; i < r.size(); ++i) CHECK(r[i] != L"C:\\s\\f7.max");
    }
    if (failures == 0) std::printf("menu spec: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
