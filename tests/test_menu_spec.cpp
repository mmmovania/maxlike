// Unit tests for the menu specification and the command table.
#include "ui/CommandTable.h"
#include "ui/MenuSpec.h"
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
    if (failures == 0) std::printf("menu spec: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
