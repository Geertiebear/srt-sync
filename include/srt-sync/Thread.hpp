#ifndef SRT_SYNC_THREAD
#define SRT_SYNC_THREAD

#include <thread>
#include <srt.h>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <srt-sync/Client.hpp>
#include <srt-sync/Decode.hpp>

namespace srt_sync {

class ServerThread {
public:
	int setup();
	int start();
	int stop();

	//TODO: dirty hack for testing
	Decode decoder;
private:
	void work();
	void handle_accept();
	void handle_client_read(int fd);
	void handle_client_write(int fd);

	struct sockaddr_in server_addr;
	SRTSOCKET socket;
	int epoll;
	std::atomic<bool> running, stopped;
	std::thread thread;
	std::unordered_map<int, std::unique_ptr<Client>> clients;
	int source_fd = 0;
};

} // namespace srt_sync

#endif // SRT_SYNC_THREAD
