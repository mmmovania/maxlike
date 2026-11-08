#pragma once
#include <cwctype>
#include <string>
#include <vector>

// The File menu's recently opened files (R3: numbered 1..9 above Exit; r3.1-menu-file.png).
namespace ui {

// `path` moved (or added) to the top; the same file twice is one entry (paths compared
// without case, as Windows does); at most `limit` entries.
inline std::vector<std::wstring> AddRecentFile(const std::vector<std::wstring>& list, const std::wstring& path, size_t limit = 9) {
    auto same = [](const std::wstring& a, const std::wstring& b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (std::towlower(a[i]) != std::towlower(b[i])) return false;
        return true;
    };
    std::vector<std::wstring> out{path};
    for (const std::wstring& p : list)
        if (!same(p, path) && out.size() < limit) out.push_back(p);
    return out;
}

} // namespace ui
