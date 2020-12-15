#ifndef SRT_SYNC_DATA_TYPES
#define SRT_SYNC_DATA_TYPES

#include <cstdint>
#include <array>

extern "C" {
#include <libavformat/avformat.h>
}

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

struct AVFrameWrapper {
	// default constructor
	AVFrameWrapper() {
		video_frame = av_frame_alloc();
		audio_frame = av_frame_alloc();
	}
	// deallocate frame when it goes out of scope
	~AVFrameWrapper() {
		if (video_frame)
			av_frame_free(&video_frame);
		if (audio_frame)
			av_frame_free(&audio_frame);
	}
	// no copying frames -- TODO: this can be implemented
	// when needed
	AVFrameWrapper(const AVFrameWrapper&) = delete;
	// but we can move them
	AVFrameWrapper(AVFrameWrapper &&other) {
		video_frame = other.video_frame;
		other.video_frame = nullptr;

		audio_frame = other.audio_frame;
		other.audio_frame = nullptr;
	}

	AVFrame *video_frame;
	AVFrame *audio_frame;
};

} // namespace srt_sync

#endif // SRT_SYNC_DATA_TYPES
