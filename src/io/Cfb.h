#pragma once
#include <cstdint>
#include <string>
#include <vector>

// A Microsoft Compound File Binary (OLE2) reader: the container of a .max file
// (docs/reference/max-format.md §1). Reads from memory; no Windows dependency
// (unit-tested in tests/test_cfb.cpp against files written by an independent
// implementation). Every offset is bounds-checked: a damaged file gives an error, never
// a crash.
namespace io {

class CompoundFile {
public:
    // Parse the header, FAT (with DIFAT), mini FAT and directory. On failure returns false
    // and sets `error`.
    bool Open(std::vector<uint8_t> bytes, std::string& error);

    // Names of the streams directly under the root, in directory order (UTF-8; Max's
    // property sets start with "\x05").
    std::vector<std::string> StreamNames() const;

    // A root-level stream's bytes; false if there is no such stream or it is damaged.
    bool ReadStream(const std::string& name, std::vector<uint8_t>& out, std::string& error) const;

private:
    struct Entry {
        std::string name;
        uint8_t type = 0;           // 1 storage, 2 stream, 5 root
        uint32_t left = kNone, right = kNone, child = kNone;
        uint32_t start = 0;
        uint64_t size = 0;
    };
    static constexpr uint32_t kNone = 0xFFFFFFFFu;
    static constexpr uint32_t kEndOfChain = 0xFFFFFFFEu;

    bool Chain(const std::vector<uint32_t>& fat, uint32_t start, std::vector<uint32_t>& out, std::string& error) const;
    bool ReadRegular(uint32_t start, uint64_t size, std::vector<uint8_t>& out, std::string& error) const;
    bool ReadMini(uint32_t start, uint64_t size, std::vector<uint8_t>& out, std::string& error) const;
    void CollectChildren(uint32_t id, std::vector<uint32_t>& out, int depth) const;

    std::vector<uint8_t> bytes_;
    uint32_t sectorSize_ = 512, miniSectorSize_ = 64, miniCutoff_ = 4096;
    std::vector<uint32_t> fat_, miniFat_;
    std::vector<Entry> entries_;
    std::vector<uint8_t> miniStream_;
};

} // namespace io
