#include <srt-sync/Thread.hpp>
#include <iostream>
#include <array>
#include <stdexcept>

namespace srt_sync {

int ServerThread::setup() {
	epoll = srt_epoll_create();
	if (epoll < 0) {
		// TODO:: LOGGING
		return -1;
	}

	socket = srt_create_socket();
	if (socket == SRT_ERROR) {
		//TODO: logging
		std::cout << "srt_socket: " << srt_getlasterror_str() << std::endl;
		return -1;
	}

	std::cout << "socket: " << socket << std::endl;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(10000);
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

	int no = 0;
	srt_setsockflag(socket, SRTO_RCVSYN, &no, sizeof(no));

	int ret = srt_bind(socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr));
	if (ret == SRT_ERROR) {
		//TODO: logging
		std::cout << "srt_bind: " << srt_getlasterror_str() << std::endl;
		return -1;
	}

	int modes = SRT_EPOLL_IN | SRT_EPOLL_ERR | SRT_EPOLL_ET;
	ret = srt_epoll_add_usock(epoll, socket, &modes);
	if (ret == SRT_ERROR) {
		//TODO: logging
		std::cout << "srt_epoll_add_usock: " << srt_getlasterror_str() << std::endl;
		return -1;
	}

	ret = srt_listen(socket, 5);
	return 0;
}

int ServerThread::start() {
	running = true;
	thread = std::thread(&ServerThread::work, this);
	try {
		thread.detach();
	} catch (std::system_error &e) {
		// TODO: logging
		throw std::runtime_error("Failed to detach thread!");
	}
	return 0;
}

int ServerThread::stop() {
	running = false;
	while (!stopped);

	srt_close(socket);
	srt_epoll_clear_usocks(epoll);
	srt_epoll_release(epoll);
	return 0;
}

void ServerThread::work() {
	stopped = false;
	while (running) {
		std::array<SRT_EPOLL_EVENT, 128> events;
		int ret = srt_epoll_uwait(epoll, events.data(), events.size(), 10);
		if (!ret)
			continue;

		for (int i = 0; i < ret; i++) {
			auto &event = events[i];
			switch(event.events) {
				case SRT_EPOLL_IN:
					if (event.fd == socket)
						handle_accept();
					else
						handle_client_read(event.fd);
					break;
				case SRT_EPOLL_ERR:
					std::cout << "removing socket " << std::endl;
					clients.erase(event.fd);
					srt_close(event.fd);
					break;
				case SRT_EPOLL_OUT:
					std::cout << "EPOLL OUT TRIGGERED" << std::endl;
					handle_client_write(event.fd);
					break;
				default:
					std::cout << "unexpected event triggered";
					break;
			}
		}
	}
	stopped = true;
}

void ServerThread::handle_accept() {
	struct sockaddr_in their_addr = {0};
	int addr_size = sizeof(their_addr);
	int their_fd = srt_accept(socket, reinterpret_cast<struct sockaddr*>(&their_addr), &addr_size);

	std::string stream_id;
	int stream_id_len = 512;
	stream_id.resize(512);
	int ret = srt_getsockflag(their_fd, SRTO_STREAMID, stream_id.data(), &stream_id_len);
	if (ret == SRT_ERROR) {
		//TODO: logging
		std::cout << "srt_getsockflag: " << srt_getlasterror_str() << std::endl;
		return;
	}
	stream_id.resize(stream_id_len);

	std::unique_ptr<Client> client;
	int modes = SRT_EPOLL_ERR;
	if (stream_id == "push") {
		source_fd = their_fd;
		client = std::make_unique<Client>(their_fd, NodeTypes::SOURCE);
		modes |= SRT_EPOLL_IN | SRT_EPOLL_ET;

		int no = 0;
		srt_setsockflag(their_fd, SRTO_RCVSYN, &no, sizeof(no));
		auto sink = decoder.get_sink();
		auto source = client->get_source();
		decoder.set_from(source->make_connection());
		client->set_to(sink->make_connection());
	} else if (stream_id == "pull") {
		client = std::make_unique<Client>(their_fd, NodeTypes::SINK);
		modes |= SRT_EPOLL_OUT | SRT_EPOLL_ET;

		/* if (source_fd) {
			auto sink = client->get_sink();
			auto &source_client = clients[source_fd];
			auto source = source_client->get_source();
			client->set_from(source->make_connection());
			source_client->set_to(sink->make_connection());
		} */
		int no = 0;
		srt_setsockflag(their_fd, SRTO_SNDSYN, &no, sizeof(no));
	} else {
		// TODO: logging
		std::cout << "got invalid streamid: " << stream_id << " and dropped client" << std::endl;
		srt_close(their_fd);
		return;
	}

	ret = srt_epoll_add_usock(epoll, their_fd, &modes);
	if (ret == SRT_ERROR) {
		//TODO: logging
		std::cout << "srt_epoll_add_usock: " << srt_getlasterror_str() << std::endl;
		return;
	}

	clients[their_fd] = std::move(client);
}

void ServerThread::handle_client_read(int fd) {
	auto &client = clients[fd];
	auto source = client->get_source();
	if (!source) {
		std::cout << "warning: SRT_EPOLL_IN enabled on socket with no sink" << std::endl;
		return;
	}
	int ret = source->raise_ready();

	decoder.get_sink()->raise_ready();
}

void ServerThread::handle_client_write(int fd) {
	auto &client = clients[fd];
	auto sink = client->get_sink();
	if (!sink) {
		std::cout << "warning: SRT_EPOLL_OUT enabled on socket with no sink" << std::endl;
		return;
	}

	int ret = sink->raise_ready();
}

} // namespace srt_sync
