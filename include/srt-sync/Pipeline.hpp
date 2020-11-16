#ifndef SRT_SYNC_PIPELINE
#define SRT_SYNC_PIPELINE

#include <srt-sync/DataTypes.hpp>
#include <optional>
#include <memory>

namespace srt_sync {

enum class NodeTypes {
	SINK,
	SOURCE,
	SINK_SOURCE,
};

template<typename T, NodeTypes type>
class NodePort;

template<typename T, NodeTypes Type>
class Connection {
public:
	using PortType = NodePort<T, Type>;
	Connection()
	: port(nullptr)
	{}

	Connection(PortType *port)
	: port(port)
	{}

	constexpr bool connected() const {
		return port != nullptr;
	}

	void disconnect() {
		port = nullptr;
	}

	bool ready() {
		if (connected())
			return port->ready();
		return false;
	}

	// TODO: maybe hide these for non-source and sinks?
	int push(T &&data) {
		if constexpr (Type == NodeTypes::SINK)
			return port->push(std::move(data));
		else
			return -1;
	}

	std::optional<T> pull() {
		if constexpr (Type == NodeTypes::SOURCE)
			return port->pull();
		else
			return std::nullopt;
	}
private:
	PortType *port;
};

template<typename T>
class NodePort<T, NodeTypes::SINK> {
public:
	using ConnectionType = Connection<T, NodeTypes::SINK>;

	virtual ~NodePort() {
		if(current_connection)
			current_connection->disconnect();
	}
	// Check to see if the node is ready to
	// for a push.
	virtual bool ready() = 0;

	// Signal to the port new data is ready
	virtual int raise_ready() = 0;

	// If ready for a push, will process
	// new object
	virtual int push(T &&data) = 0;

	std::shared_ptr<ConnectionType> make_connection() {
		auto connection = std::make_shared<ConnectionType>(this);
		if (current_connection)
			current_connection->disconnect();
		current_connection = connection;
		return connection;
	}
private:
	std::shared_ptr<ConnectionType> current_connection;
};

template<typename T>
class NodePort<T, NodeTypes::SOURCE> {
public:
	using ConnectionType = Connection<T, NodeTypes::SOURCE>;

	virtual ~NodePort() {
		if(current_connection)
			current_connection->disconnect();
	}

	// Check to see if the node is ready to
	// for a pull.
	virtual bool ready() = 0;

	// Signal to the port new data is ready
	virtual int raise_ready() = 0;

	// If ready for a pull, will return a new
	// object.
	virtual std::optional<T> pull() = 0;

	std::shared_ptr<ConnectionType> make_connection() {
		auto connection = std::make_shared<ConnectionType>(this);
		if (current_connection)
			current_connection->disconnect();
		current_connection = connection;
		return connection;
	}
private:
	std::shared_ptr<ConnectionType> current_connection;
};

template <typename In, typename Out>
class Node {
public:
	using Sink = NodePort<In, NodeTypes::SINK>;
	using Source = NodePort<Out, NodeTypes::SOURCE>;
	Node(Sink *node_sink, Source *node_source,
			NodeTypes node_type)
	: node_sink(node_sink), node_source(node_source), node_type(node_type)
	{ }

	inline Sink* get_sink() {
		if (node_type == NodeTypes::SINK || node_type == NodeTypes::SINK_SOURCE)
			return node_sink;
		return nullptr;
	}

	inline Source* get_source() {
		if (node_type == NodeTypes::SOURCE || node_type == NodeTypes::SINK_SOURCE)
			return node_source;
		return nullptr;
	}
protected:
	Sink *node_sink;
	Source *node_source;
	NodeTypes node_type;
};

} // namespace srt_sync

#endif // SRT_SYNC_PIPELINE
