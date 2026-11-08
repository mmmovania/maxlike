// The R3.1 sample files (docs/reference/samples/*.max) through the container layers:
// every file opens, has the streams a .max file has, and every chunk stream parses
// completely. The Scene stream is one top-level container, type 0x2004
// (docs/reference/max-format.md §3).
#include "io/Cfb.h"
#include "io/Chunks.h"
#include <cstdio>
#include <fstream>
#include <iterator>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s [%s]\n", __FILE__, __LINE__, #cond, file); } } while (0)

static const char* const kSamples[] = {
    "box", "box_editmesh", "boxes", "cone", "cylinder", "geosphere", "instances", "parent_child_link", "pivot",
    "plane", "pyramid", "sphere", "teapot", "teapot_edit_mesh", "teapot_smoothinggroups", "torus", "tube"};

int main() {
    for (const char* file : kSamples) {
        std::ifstream f(std::string(MAXLIKE_SAMPLES) + "/" + file + ".max", std::ios::binary);
        const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        CHECK(!bytes.empty());
        io::CompoundFile cf;
        std::string error;
        CHECK(cf.Open(bytes, error));
        const auto names = cf.StreamNames();
        for (const char* required : {"Scene", "ClassDirectory3", "DllDirectory", "Config", "ClassData", "VideoPostQueue",
                                     "\x05SummaryInformation", "\x05" "DocumentSummaryInformation"}) {
            bool found = false;
            for (const auto& n : names) found = found || n == required;
            CHECK(found);
        }
        for (const auto& n : names) {
            if (n.empty() || n[0] < 32) continue;                       // OLE property sets are not chunks
            std::vector<uint8_t> s;
            CHECK(cf.ReadStream(n, s, error));
            std::vector<io::Chunk> chunks;
            CHECK(io::ParseChunks(s, 0, s.size(), chunks, error));
            if (n == "Scene") CHECK(chunks.size() == 1 && chunks[0].type == 0x2004 && chunks[0].container);
        }
    }
    if (failures == 0) std::printf("samples: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
