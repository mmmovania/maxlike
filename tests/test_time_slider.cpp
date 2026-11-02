// Unit tests for the time slider geometry. Expected values measured from the R3.1
// reference screenshot at frame 0 (100%).
#include "ui/TimeSliderGeometry.h"
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

using namespace ui::timeslider;

static void FrameZeroMatchesReference() {
    const Parts p = PartsAt(0, 0, 100, 1736, 17, 96);
    CHECK(p.prev.left == 0 && p.prev.right == 16);      // "<" at x 0..15
    CHECK(p.thumb.left == 16 && p.thumb.right == 141);  // thumb at x 16..140
    CHECK(p.next.left == 141 && p.next.right == 157);   // ">" at x 141..156
    CHECK(p.prev.top == 0 && p.prev.bottom == 17);
}

static void LastFrameReachesTheRightEdge() {
    const Parts p = PartsAt(100, 0, 100, 1736, 17, 96);
    CHECK(p.next.right == 1736);
    CHECK(PartsAt(250, 0, 100, 1736, 17, 96).next.right == 1736); // clamped
}

static void BlockLeftRoundTrips() {
    for (int f = 0; f <= 100; ++f) {
        const Parts p = PartsAt(f, 0, 100, 1736, 17, 96);
        CHECK(FrameAtBlockLeft(p.prev.left, 0, 100, 1736, 96) == f);
    }
    CHECK(FrameAtBlockLeft(-50, 0, 100, 1736, 96) == 0);      // clamped
    CHECK(FrameAtBlockLeft(99999, 0, 100, 1736, 96) == 100);
}

static void OtherRangesAndDpi() {
    // A range that does not start at 0.
    CHECK(PartsAt(10, 10, 20, 1000, 17, 96).prev.left == 0);
    CHECK(PartsAt(20, 10, 20, 1000, 17, 96).next.right == 1000);
    // At 150% the block is 236 px wide (16*1.5 + 125*1.5 = 24 + 188 (187.5 rounds up) + 24).
    const Parts p = PartsAt(0, 0, 100, 2604, 26, 144);
    CHECK(p.next.right - p.prev.left == 24 + 188 + 24);
    // A slider narrower than the block never produces negative positions.
    CHECK(PartsAt(50, 0, 100, 100, 17, 96).prev.left == 0);
}

int main() {
    FrameZeroMatchesReference();
    LastFrameReachesTheRightEdge();
    BlockLeftRoundTrips();
    OtherRangesAndDpi();
    if (failures == 0) std::printf("time slider: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
