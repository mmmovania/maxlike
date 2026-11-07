#include "io/Cfb.h"
#include <algorithm>

namespace io {

namespace {

uint16_t U16(const std::vector<uint8_t>& b, size_t o) { return static_cast<uint16_t>(b[o] | (b[o + 1] << 8)); }
uint32_t U32(const std::vector<uint8_t>& b, size_t o) {
    return static_cast<uint32_t>(b[o]) | (static_cast<uint32_t>(b[o + 1]) << 8) | (static_cast<uint32_t>(b[o + 2]) << 16) |
           (static_cast<uint32_t>(b[o + 3]) << 24);
}

// UTF-16LE (as stored, without the terminator) to UTF-8.
std::string Utf8(const std::vector<uint16_t>& w) {
    std::string s;
    for (size_t i = 0; i < w.size(); ++i) {
        uint32_t c = w[i];
        if (c >= 0xD800 && c < 0xDC00 && i + 1 < w.size() && w[i + 1] >= 0xDC00 && w[i + 1] < 0xE000)
            c = 0x10000 + ((c - 0xD800) << 10) + (w[++i] - 0xDC00);
        if (c < 0x80) s += static_cast<char>(c);
        else if (c < 0x800) { s += static_cast<char>(0xC0 | (c >> 6)); s += static_cast<char>(0x80 | (c & 0x3F)); }
        else if (c < 0x10000) { s += static_cast<char>(0xE0 | (c >> 12)); s += static_cast<char>(0x80 | ((c >> 6) & 0x3F)); s += static_cast<char>(0x80 | (c & 0x3F)); }
        else { s += static_cast<char>(0xF0 | (c >> 18)); s += static_cast<char>(0x80 | ((c >> 12) & 0x3F)); s += static_cast<char>(0x80 | ((c >> 6) & 0x3F)); s += static_cast<char>(0x80 | (c & 0x3F)); }
    }
    return s;
}

} // namespace

bool CompoundFile::Open(std::vector<uint8_t> bytes, std::string& error) {
    bytes_ = std::move(bytes);
    fat_.clear(); miniFat_.clear(); entries_.clear(); miniStream_.clear();
    static const uint8_t kSignature[8] = {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};
    if (bytes_.size() < 512) { error = "too small for a compound file"; return false; }
    for (int i = 0; i < 8; ++i)
        if (bytes_[static_cast<size_t>(i)] != kSignature[i]) { error = "not a compound file (signature)"; return false; }
    const uint16_t shift = U16(bytes_, 0x1E), miniShift = U16(bytes_, 0x20);
    if (shift < 7 || shift > 16 || miniShift > shift) { error = "invalid sector size"; return false; }
    sectorSize_ = 1u << shift;
    miniSectorSize_ = 1u << miniShift;
    miniCutoff_ = U32(bytes_, 0x38);
    const uint32_t fatSectors = U32(bytes_, 0x2C), firstDir = U32(bytes_, 0x30);
    const uint32_t firstMiniFat = U32(bytes_, 0x3C), miniFatSectors = U32(bytes_, 0x40);
    uint32_t difat = U32(bytes_, 0x44), difatSectors = U32(bytes_, 0x48);
    const uint64_t sectorCount = (bytes_.size() - 1) / sectorSize_;   // sector n starts at (n + 1) * size
    if (fatSectors > sectorCount) { error = "FAT larger than the file"; return false; }

    // The FAT's sector numbers: 109 in the header, the rest in the DIFAT chain.
    std::vector<uint32_t> fatSectorList;
    for (uint32_t i = 0; i < 109 && fatSectorList.size() < fatSectors; ++i) fatSectorList.push_back(U32(bytes_, 0x4C + 4 * i));
    const uint32_t perDifat = sectorSize_ / 4 - 1;
    while (fatSectorList.size() < fatSectors && difat < kEndOfChain && difatSectors-- > 0) {
        if (!WholeSector(difat)) { error = "DIFAT sector outside the file"; return false; }
        const size_t base = (static_cast<size_t>(difat) + 1) * sectorSize_;
        for (uint32_t i = 0; i < perDifat && fatSectorList.size() < fatSectors; ++i) fatSectorList.push_back(U32(bytes_, base + 4 * i));
        difat = U32(bytes_, base + 4 * perDifat);
    }
    if (fatSectorList.size() < fatSectors) { error = "incomplete FAT sector list"; return false; }
    for (uint32_t s : fatSectorList) {
        if (!WholeSector(s)) { error = "FAT sector outside the file"; return false; }
        const size_t base = (static_cast<size_t>(s) + 1) * sectorSize_;
        for (uint32_t i = 0; i < sectorSize_ / 4; ++i) fat_.push_back(U32(bytes_, base + 4 * i));
    }

    // Directory: 128-byte entries along the directory chain.
    std::vector<uint32_t> dirChain;
    if (!Chain(fat_, firstDir, dirChain, error)) return false;
    for (uint32_t s : dirChain) {
        if (!WholeSector(s)) { error = "directory sector outside the file"; return false; }
        const size_t base = (static_cast<size_t>(s) + 1) * sectorSize_;
        for (uint32_t e = 0; e < sectorSize_ / 128; ++e) {
            const size_t o = base + 128 * e;
            Entry entry;
            const uint16_t nameBytes = U16(bytes_, o + 64);
            std::vector<uint16_t> w;
            for (uint32_t i = 0; i + 2 < nameBytes && i < 64; i += 2) w.push_back(U16(bytes_, o + i));   // without the terminator
            entry.name = Utf8(w);
            entry.type = bytes_[o + 66];
            entry.left = U32(bytes_, o + 68);
            entry.right = U32(bytes_, o + 72);
            entry.child = U32(bytes_, o + 76);
            entry.start = U32(bytes_, o + 116);
            entry.size = U32(bytes_, o + 120);
            if (sectorSize_ > 512) entry.size |= static_cast<uint64_t>(U32(bytes_, o + 124)) << 32;   // high part valid only then
            entries_.push_back(entry);
        }
    }
    if (entries_.empty() || entries_[0].type != 5) { error = "no root entry"; return false; }

    // Mini FAT and the mini stream (the root entry's chain).
    std::vector<uint32_t> miniFatChain;
    if (miniFatSectors > 0) {
        if (!Chain(fat_, firstMiniFat, miniFatChain, error)) return false;
        for (uint32_t s : miniFatChain) {
            if (!WholeSector(s)) { error = "mini FAT sector outside the file"; return false; }
            const size_t base = (static_cast<size_t>(s) + 1) * sectorSize_;
            for (uint32_t i = 0; i < sectorSize_ / 4; ++i) miniFat_.push_back(U32(bytes_, base + 4 * i));
        }
        if (!ReadRegular(entries_[0].start, entries_[0].size, miniStream_, error)) return false;
    }
    return true;
}

// Follow a sector chain; detects loops and sectors outside the file.
bool CompoundFile::Chain(const std::vector<uint32_t>& fat, uint32_t start, std::vector<uint32_t>& out, std::string& error) const {
    out.clear();
    for (uint32_t s = start; s != kEndOfChain;) {
        if (s >= fat.size()) { error = "sector chain leaves the FAT"; return false; }
        if (out.size() > fat.size()) { error = "sector chain loops"; return false; }
        out.push_back(s);
        s = fat[s];
    }
    return true;
}

bool CompoundFile::ReadRegular(uint32_t start, uint64_t size, std::vector<uint8_t>& out, std::string& error) const {
    std::vector<uint32_t> chain;
    if (size > 0 && !Chain(fat_, start, chain, error)) return false;
    if (static_cast<uint64_t>(chain.size()) * sectorSize_ < size) { error = "stream shorter than its size"; return false; }
    out.clear();
    out.reserve(static_cast<size_t>(size));
    for (uint32_t s : chain) {
        const size_t base = (static_cast<size_t>(s) + 1) * sectorSize_;
        const size_t n = static_cast<size_t>(std::min<uint64_t>(sectorSize_, size - out.size()));
        if (base + n > bytes_.size()) { error = "sector outside the file"; return false; }   // exactly the bytes needed
        out.insert(out.end(), bytes_.begin() + static_cast<std::ptrdiff_t>(base), bytes_.begin() + static_cast<std::ptrdiff_t>(base + n));
        if (out.size() == size) break;
    }
    return true;
}

bool CompoundFile::ReadMini(uint32_t start, uint64_t size, std::vector<uint8_t>& out, std::string& error) const {
    std::vector<uint32_t> chain;
    if (size > 0 && !Chain(miniFat_, start, chain, error)) return false;
    out.clear();
    for (uint32_t s : chain) {
        const size_t base = static_cast<size_t>(s) * miniSectorSize_;
        const size_t n = static_cast<size_t>(std::min<uint64_t>(miniSectorSize_, size - out.size()));
        if (base + n > miniStream_.size()) { error = "mini sector outside the mini stream"; return false; }
        out.insert(out.end(), miniStream_.begin() + static_cast<std::ptrdiff_t>(base), miniStream_.begin() + static_cast<std::ptrdiff_t>(base + n));
        if (out.size() == size) break;
    }
    if (out.size() < size) { error = "mini stream shorter than its size"; return false; }
    return true;
}

// The children of a storage are a red-black tree (left / right siblings, from `child`);
// in-order gives directory order.
void CompoundFile::CollectChildren(uint32_t id, std::vector<uint32_t>& out, int depth) const {
    if (id == kNone || id >= entries_.size() || depth > 64 || out.size() > entries_.size()) return;
    CollectChildren(entries_[id].left, out, depth + 1);
    out.push_back(id);
    CollectChildren(entries_[id].right, out, depth + 1);
}

std::vector<std::string> CompoundFile::StreamNames() const {
    std::vector<std::string> names;
    if (entries_.empty()) return names;
    std::vector<uint32_t> ids;
    CollectChildren(entries_[0].child, ids, 0);
    for (uint32_t id : ids)
        if (entries_[id].type == 2) names.push_back(entries_[id].name);
    return names;
}

bool CompoundFile::ReadStream(const std::string& name, std::vector<uint8_t>& out, std::string& error) const {
    if (entries_.empty()) { error = "no file open"; return false; }
    std::vector<uint32_t> ids;
    CollectChildren(entries_[0].child, ids, 0);
    for (uint32_t id : ids) {
        const Entry& e = entries_[id];
        if (e.type != 2 || e.name != name) continue;
        return e.size < miniCutoff_ ? ReadMini(e.start, e.size, out, error) : ReadRegular(e.start, e.size, out, error);
    }
    error = "no stream named \"" + name + "\"";
    return false;
}

} // namespace io
