#ifndef SRT_SYNC_CLIENT
#define SRT_SYNC_CLIENT

#include <deque>
#include <span>
#include <functional>
#include <optional>
#include <srt-sync/DataTypes.hpp>
#include <srt-sync/Pipeline.hpp>

namespace srt_sync {

class Client : public Node<Frame, Frame> {
public:
	template<NodeTypes Type>
	using ConnectionType = std::shared_ptr<Connection<Frame, Type>>;

	class ClientSink : public NodePort<Frame, NodeTypes::SINK> {
	public:
		ClientSink(Client &client)
		: client(client)
		{ }

		bool ready();
		int raise_ready();
		int push(Frame &&data);
	private:
		struct Chunk {
			Frame frame;
			int pos;
		};

		int send_multiple(const std::function<std::optional<Chunk>()> &next_chunk);
		int send(const Chunk &chunk);

		Client &client;
		std::deque<Chunk> send_buffer;
	};

	class ClientSource : public NodePort<Frame, NodeTypes::SOURCE> {
	public:
		using ReadResult = std::pair<Frame, int>;
		ClientSource(Client &client)
		: client(client)
		{ }

		bool ready();
		int raise_ready();
		std::optional<Frame> pull();
	private:
		std::optional<Frame> read();
		Client &client;
		std::deque<Frame> frame_buffer;
	};

	void set_to(ConnectionType<NodeTypes::SINK> &&what) {
		to = std::move(what);
	}

	void set_from(ConnectionType<NodeTypes::SOURCE> &&what) {
		from = std::move(what);
	}

	Client(int fd, NodeTypes type)
	: Node(&sink, &source, type), fd(fd), sink(*this), source(*this)
	{ }
private:
	ConnectionType<NodeTypes::SINK> to;
	ConnectionType<NodeTypes::SOURCE> from;

	int fd;
	ClientSink sink;
	ClientSource source;
};

} // namespace srt_sync

#endif // SRT_SYNC_CLIENT
