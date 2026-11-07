// Unit tests for the chunk walker: byte-built trees, the 64-bit header, damaged sizes.
#include "io/Chunks.h"
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

using Bytes = std::vector<uint8_t>;
static void U16(Bytes& b, uint16_t v) { b.push_back(v & 0xFF); b.push_back(v >> 8); }
static void U32(Bytes& b, uint32_t v) { for (int i = 0; i < 4; ++i) b.push_back((v >> (8 * i)) & 0xFF); }
static void U64(Bytes& b, uint64_t v) { for (int i = 0; i < 8; ++i) b.push_back((v >> (8 * i)) & 0xFF); }
static Bytes Leaf(uint16_t type, const Bytes& payload) {
    Bytes b; U16(b, type); U32(b, static_cast<uint32_t>(6 + payload.size())); b.insert(b.end(), payload.begin(), payload.end()); return b;
}
static Bytes Container(uint16_t type, const Bytes& children) {
    Bytes b; U16(b, type); U32(b, 0x80000000u | static_cast<uint32_t>(6 + children.size())); b.insert(b.end(), children.begin(), children.end()); return b;
}
static Bytes Leaf64(uint16_t type, const Bytes& payload) {
    Bytes b; U16(b, type); U32(b, 0); U64(b, 14 + payload.size()); b.insert(b.end(), payload.begin(), payload.end()); return b;
}
static Bytes Cat(std::initializer_list<Bytes> parts) { Bytes b; for (const Bytes& p : parts) b.insert(b.end(), p.begin(), p.end()); return b; }

int main() {
    std::string error;
    // A container holding a leaf and a container (which holds a 64-bit leaf), then a leaf.
    const Bytes data = Cat({Container(0x2000, Cat({Leaf(0x0100, {1, 2, 3, 4}), Container(0x2001, Leaf64(0x0200, {9, 8, 7}))})),
                            Leaf(0x0300, {})});
    std::vector<io::Chunk> top;
    CHECK(io::ParseChunks(data, 0, data.size(), top, error));
    CHECK(top.size() == 2 && top[0].type == 0x2000 && top[0].container && top[1].type == 0x0300 && !top[1].container);
    CHECK(top[1].end == top[1].begin);                                      // an empty payload
    CHECK(top[0].children.size() == 2);
    const io::Chunk& leaf = top[0].children[0];
    CHECK(leaf.type == 0x0100 && leaf.end - leaf.begin == 4 && data[leaf.begin] == 1 && data[leaf.end - 1] == 4);
    const io::Chunk& inner = top[0].children[1];
    CHECK(inner.container && inner.children.size() == 1);
    const io::Chunk& big = inner.children[0];
    CHECK(big.type == 0x0200 && !big.container && big.end - big.begin == 3 && data[big.begin] == 9);
    // A container in the 64-bit form (bit 63).
    {
        Bytes b; U16(b, 0x2100); U32(b, 0); U64(b, (1ull << 63) | (14 + 6)); const Bytes child = Leaf(0x0001, {});
        b.insert(b.end(), child.begin(), child.end());
        CHECK(io::ParseChunks(b, 0, b.size(), top, error) && top.size() == 1 && top[0].container && top[0].children.size() == 1);
    }
    // Damaged: a size past the parent, a size smaller than the header, a cut header.
    Bytes bad = Leaf(0x0100, {1, 2, 3});
    bad[2] = 50;
    CHECK(!io::ParseChunks(bad, 0, bad.size(), top, error));
    bad = Leaf(0x0100, {}); bad[2] = 3;
    CHECK(!io::ParseChunks(bad, 0, bad.size(), top, error));
    bad = Bytes{0x00, 0x01, 0x10};
    CHECK(!io::ParseChunks(bad, 0, bad.size(), top, error));
    // Nesting deeper than allowed is refused instead of recursing without end.
    Bytes deep = Leaf(0x0001, {});
    for (int i = 0; i < 70; ++i) deep = Container(0x2000, deep);
    CHECK(!io::ParseChunks(deep, 0, deep.size(), top, error) && error.find("deep") != std::string::npos);
    if (failures == 0) std::printf("chunks: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
