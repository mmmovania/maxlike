#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// The chunk trees inside a .max file's streams (docs/reference/max-format.md §2.1):
//   u16 type; u32 size (including this header; bit 31 set = container of child chunks);
//   if size == 0: u64 size (bit 63 set = container); header 6 or 14 bytes.
// No Windows dependency (unit-tested in tests/test_chunks.cpp). Bounds-checked.
namespace io {

struct Chunk {
    uint16_t type = 0;
    bool container = false;
    size_t begin = 0, end = 0;          // the payload, as offsets into the stream
    std::vector<Chunk> children;        // parsed for containers
};

// Parse the chunks in data[begin, end). Containers are parsed recursively (at most
// `maxDepth` levels). False and `error` on a malformed chunk.
bool ParseChunks(const std::vector<uint8_t>& data, size_t begin, size_t end, std::vector<Chunk>& out,
                 std::string& error, int maxDepth = 64);

} // namespace io
