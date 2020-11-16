#ifndef SRT_SYNC_DATA_TYPES
#define SRT_SYNC_DATA_TYPES

#include <cstdint>
#include <array>

namespace srt_sync {

// This constant is set by the SRT live stream spec
// as the absolute maximum payload size.
static constexpr size_t MAX_FRAME_SIZE = 1456;

struct Frame {
	using ArrayType = std::array<char, MAX_FRAME_SIZE>;
	ArrayType data;
	size_t size;
	uint64_t time;
};

} // namespace srt_sync

#endif // SRT_SYNC_DATA_TYPES
