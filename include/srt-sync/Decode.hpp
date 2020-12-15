#ifndef SRT_SYNC_DECODE
#define SRT_SYNC_DECODE

#include <srt-sync/Pipeline.hpp>
#include <srt-sync/DataTypes.hpp>
#include <mutex>

// what the hell ffmpeg
extern "C" {
#include <libavformat/avformat.h>
}

namespace srt_sync {

class Decode : public Node<Frame, AVFrameWrapper> {
public:
	using ConnectionIn = std::shared_ptr<Connection<Frame, NodeTypes::SOURCE>>;
	using ConnectionOut = std::shared_ptr<Connection<AVFrameWrapper, NodeTypes::SINK>>;

	class DecodeSink : public NodePort<Frame, NodeTypes::SINK> {
	public:
		DecodeSink(Decode &decoder);

		bool ready() {
			return false;
		}
		int raise_ready();
		int push(Frame &&data) {
			return -1;
		}
	public:
		static int read_callback(void *data, uint8_t *buf, int buf_size);

		AVIOContext *io_context;
		AVInputFormat *fmt;
		AVFormatContext *fmt_context;
		Decode &decoder;
	private:
		AVPacket *packet;
	};

	class DecodeSource : public NodePort<AVFrameWrapper, NodeTypes::SOURCE> {
	public:
		DecodeSource(Decode &decoder)
		: decoder(decoder)
		{ }

		bool ready() {
			return true;
		}
		int raise_ready() {
			return 0;
		}
		std::optional<AVFrameWrapper> pull();
	private:
		Decode &decoder;
	};

	void set_to(ConnectionOut &&what) {
		to = std::move(what);
	}

	void set_from(ConnectionIn &&what) {
		from = std::move(what);
	}

	void probe_input();

	Decode()
	: Node(&sink, &source, NodeTypes::SINK_SOURCE), sink(*this), source(*this)
	{ }
public:
	ConnectionOut to;
	ConnectionIn from;

	AVCodecContext *video_decode_context, *audio_decode_context;
	int audio_index, video_index;
	DecodeSink sink;
	DecodeSource source;
	bool initialized = false;

	std::optional<int> open_codec_context(AVCodecContext **decode_context, enum AVMediaType type);
};

} // namespace srt_sync

#endif // SRT_SYNC_DECODE
