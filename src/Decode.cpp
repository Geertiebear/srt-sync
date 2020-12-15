#include <srt-sync/Decode.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace srt_sync {

Decode::DecodeSink::DecodeSink(Decode &decoder)
	: decoder(decoder) {
	auto buffer = static_cast<unsigned char*>(av_malloc(MAX_FRAME_SIZE));
	if (!buffer)
		return;

	io_context = avio_alloc_context(buffer, MAX_FRAME_SIZE,
			0, this, &read_callback, nullptr, nullptr);
	if (!io_context)
		return;

	fmt_context = avformat_alloc_context();
	if(!fmt_context)
		return;

	fmt_context->pb = io_context;

	packet = av_packet_alloc();
}

int Decode::DecodeSink::read_callback(void *data, uint8_t *buf, int buf_size) {
	auto self = static_cast<DecodeSink*>(data);
	if (!self->decoder.from || !self->decoder.from->connected())
		return -1;

	auto frame = self->decoder.from->pull();
	if (!frame)
		return AVERROR(EAGAIN);
	auto to_copy = std::min(buf_size, static_cast<int>((*frame).size));
	memcpy(buf, (*frame).data.data(), to_copy);
	return to_copy;
}

int Decode::DecodeSink::raise_ready() {
	if (!decoder.initialized)
		return 0;
	packet->data = NULL;
	packet->size = 0;

	if (!decoder.from->ready())
		return 0;

	if(av_read_frame(fmt_context, packet) < 0)
		return 0;

	int ret = 0;
	if (packet->stream_index == decoder.video_index)
		ret = avcodec_send_packet(decoder.video_decode_context, packet);
	else if (packet->stream_index == decoder.audio_index)
		ret = avcodec_send_packet(decoder.audio_decode_context, packet);

	av_packet_unref(packet);
	if (ret < 0)
		return -1;

	return 0;
}

std::optional<AVFrameWrapper> Decode::DecodeSource::pull() {
	if (!decoder.initialized)
		return std::nullopt;
	AVFrameWrapper wrapper;

	int ret = avcodec_receive_frame(decoder.video_decode_context, wrapper.video_frame);
	if (ret < 0)
		return std::nullopt;

	ret = avcodec_receive_frame(decoder.audio_decode_context, wrapper.audio_frame);
	if (ret < 0)
		return std::nullopt;

	decoder.sink.raise_ready();

	return wrapper;
}

std::optional<int> Decode::open_codec_context(AVCodecContext **decode_context, enum AVMediaType type) {
	int ret = av_find_best_stream(sink.fmt_context, type, -1, -1, NULL, 0);
	if (ret < 0) {
		std::cout << "Failed to find best stream" << std::endl;
		return std::nullopt;
	}

	int stream_idx = ret;
	AVStream *stream = sink.fmt_context->streams[stream_idx];
	AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
	if (!dec) {
		std::cout << "Failed finding decoder for codec_id " <<
			stream->codecpar->codec_id << std::endl;
		return std::nullopt;
	}

	*decode_context = avcodec_alloc_context3(dec);
	if (!decode_context) {
		std::cout << "Failed to allocate decoder_context" << std::endl;
		return std::nullopt;
	}

	if (avcodec_parameters_to_context(*decode_context, stream->codecpar) < 0) {
		std::cout << "Failed to copy codec parameters" << std::endl;
		return std::nullopt;
	}

	if (avcodec_open2(*decode_context, dec, NULL) < 0) {
		std::cout << "Failed to open codec context" << std::endl;
		return std::nullopt;
	}

	return stream_idx;
}

void Decode::probe_input() {
	std::cout << "probe_input()" << std::endl;
	int ret = av_probe_input_buffer2(sink.io_context, &sink.fmt, NULL, NULL, 0, 0);
	std::cout << "av_probe_input_buffer2(): " << ret << std::endl;
	std::cout << sink.fmt->name << std::endl;

	ret = avformat_open_input(&sink.fmt_context, NULL, sink.fmt, NULL);
	std::cout << "avformat_open_input(): " << ret << std::endl;

	ret = avformat_find_stream_info(sink.fmt_context, NULL);
	std::cout << "avformat_find_stream_info(): " << ret << std::endl;

	auto index_ret = open_codec_context(&video_decode_context, AVMEDIA_TYPE_VIDEO);
	if (!index_ret) {
		std::cout << "open_codex_context() failed for video!" << std::endl;
		return;
	}

	video_index = *index_ret;

	index_ret = open_codec_context(&audio_decode_context, AVMEDIA_TYPE_AUDIO);
	if (!index_ret) {
		std::cout << "open_codex_context() failed for audio!" << std::endl;
		return;
	}

	audio_index = *index_ret;

	av_dump_format(sink.fmt_context, 0, NULL, 0);

	initialized = true;
	sink.raise_ready();
}

} // namespace srt_sync
