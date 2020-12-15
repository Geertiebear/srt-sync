#include <srt-sync/Client.hpp>
#include <srt.h>

namespace srt_sync {

bool Client::ClientSink::ready() {
	return true;
}

int Client::ClientSink::raise_ready() {
	int no_bytes = 0;

	if (!send_buffer.empty()) {
		int ret = send_multiple([this] () -> std::optional<Chunk> {
			if (send_buffer.empty())
				return std::nullopt;
			auto chunk = send_buffer.front();
			send_buffer.pop_front();
			return chunk;
		});
		if (ret == SRT_ERROR)
			return -1;
		if (ret == 0)
			return 0;
		no_bytes += ret;
	}

	if(!client.from)
		return 0;

	if(!client.from->connected() || !client.from->ready())
		return 0;

	int ret = send_multiple([this] () -> std::optional<Chunk> {
		auto frame = client.from->pull();
		if (!frame)
			return std::nullopt;
		return Chunk{std::move(*frame), 0};
	});

	if (ret == SRT_ERROR)
		return -1;

	no_bytes += ret;
	return no_bytes;
}

int Client::ClientSink::push(Frame &&data) {
	if(!send_buffer.empty()) {
		Chunk chunk {std::move(data), 0};
		send_buffer.push_back(std::move(chunk));

		int ret = send_multiple([this] () -> std::optional<Chunk> {
			if (send_buffer.empty())
				return std::nullopt;
			auto chunk = send_buffer.front();
			send_buffer.pop_front();
			return chunk;
		});
		if (ret == SRT_ERROR)
			return -1;
		if (ret == 0)
			return 0;

		return ret;
	}

	Chunk chunk{std::move(data), 0};
	int ret = send(chunk);
	if (ret == SRT_ERROR){
		send_buffer.push_back(chunk);
		return ret;
	}

	chunk.pos = ret;
	send_buffer.push_back(chunk);
	return ret;
}

int Client::ClientSink::send_multiple(const std::function<std::optional<Chunk>()> &next_chunk) {
	int no_bytes = 0;
	while (1) {
		auto chunk = next_chunk();
		if (!chunk)
			return no_bytes;
		int ret = send(*chunk);
		if (ret == SRT_ERROR) {
			if (srt_getlasterror(nullptr) == SRT_EASYNCSND)
				break;
			else
				return ret;
		}
		no_bytes += ret;
	}
	return no_bytes;
}

int Client::ClientSink::send(const Chunk &chunk) {
	int to_send = chunk.frame.size - chunk.pos;
	if (to_send == 0)
		return 0;
	SRT_MSGCTRL message_control = srt_msgctrl_default;
	message_control.srctime = srt_time_now();
	return srt_sendmsg2(client.fd, chunk.frame.data.data() + chunk.pos, to_send, &message_control);
}

bool Client::ClientSource::ready() {
	return !frame_buffer.empty();
}

int Client::ClientSource::raise_ready() {
	int no_bytes = 0;
	while (1) {
		auto frame = read();
		if (!frame) {
			int err = srt_getlasterror(nullptr);
			if (err == SRT_EASYNCRCV)
				break;
			return -1;
		}

		if(!client.to) {
			no_bytes += frame->size;
			continue;
		}

		if (!client.to->connected()) {
			no_bytes += frame->size;
			continue;
		}

		if (client.to->ready()) {
			client.to->push(std::move(*frame));
			no_bytes += frame->size;
			continue;
		}

		frame_buffer.push_back(std::move(*frame));
		no_bytes += frame->size;
	}

	return no_bytes;
}

std::optional<Frame> Client::ClientSource::pull() {
	if (frame_buffer.empty())
		return std::nullopt;

	auto frame = frame_buffer.front();
	frame_buffer.pop_front();
	return frame;
}

std::optional<Frame> Client::ClientSource::read() {
	Frame frame;
	SRT_MSGCTRL message_control = srt_msgctrl_default;
	int ret = srt_recvmsg2(client.fd, frame.data.data(), frame.data.size(), &message_control);
	if (ret == SRT_ERROR)
		return std::nullopt;

	frame.size = ret;
	frame.time = message_control.srctime;
	return frame;
}

} // namespace srt_sync
