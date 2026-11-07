// Unit tests for the compound-file reader, against files written by an independent
// implementation (SheetJS cfb 1.2.2; tests/data/cfb-*.cfb, generator in the commit
// message). Each stream's byte i is (i * mul + add) & 0xFF.
#include "io/Cfb.h"
#include <cstdio>
#include <fstream>
#include <iterator>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static std::vector<uint8_t> Load(const char* name) {
    std::ifstream f(std::string(MAXLIKE_TEST_DATA) + "/" + name, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static bool Pattern(const std::vector<uint8_t>& b, size_t n, int mul, int add) {
    if (b.size() != n) return false;
    for (size_t i = 0; i < n; ++i)
        if (b[i] != static_cast<uint8_t>((i * mul + add) & 0xFF)) return false;
    return true;
}

int main() {
    std::string error;
    // Small streams only: all in the mini stream.
    {
        io::CompoundFile cf;
        CHECK(cf.Open(Load("cfb-small.cfb"), error));
        const auto names = cf.StreamNames();
        // As SheetJS's own reader lists them (it adds its "\x01Sh33tJ5" marker stream).
        CHECK(names.size() == 4);
        CHECK(names.size() == 4 && names[0] == "Scene" && names[1] == "\x01Sh33tJ5" && names[2] == "ClassDirectory3" &&
              names[3] == "\x05SummaryInformation");
        std::vector<uint8_t> s;
        CHECK(cf.ReadStream("Scene", s, error) && Pattern(s, 100, 7, 3));
        CHECK(cf.ReadStream("ClassDirectory3", s, error) && Pattern(s, 40, 11, 5));
        CHECK(cf.ReadStream("\x05SummaryInformation", s, error) && Pattern(s, 64, 13, 1));
        CHECK(!cf.ReadStream("Nothing", s, error) && !error.empty());
    }
    // Large streams: regular sectors (10000 and 5000 bytes) next to a small one.
    {
        io::CompoundFile cf;
        CHECK(cf.Open(Load("cfb-large.cfb"), error));
        std::vector<uint8_t> s;
        CHECK(cf.ReadStream("Scene", s, error) && Pattern(s, 10000, 7, 3));
        CHECK(cf.ReadStream("Config", s, error) && Pattern(s, 5000, 5, 2));
        CHECK(cf.ReadStream("DllDirectory", s, error) && Pattern(s, 300, 3, 9));
    }
    // Damaged input gives an error, never a crash.
    {
        io::CompoundFile cf;
        CHECK(!cf.Open({1, 2, 3}, error));
        auto bytes = Load("cfb-small.cfb");
        bytes[0] = 0;
        CHECK(!cf.Open(bytes, error) && error.find("signature") != std::string::npos);
        bytes = Load("cfb-large.cfb");
        bytes.resize(2048);                                  // cut short
        std::vector<uint8_t> s;
        CHECK(!cf.Open(bytes, error) || !cf.ReadStream("Scene", s, error));
        bytes = Load("cfb-small.cfb");
        for (size_t i = 0x4C; i < 0x50; ++i) bytes[i] = 0xEE;   // the first FAT sector: far outside
        CHECK(!cf.Open(bytes, error));
    }
    if (failures == 0) std::printf("cfb: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
