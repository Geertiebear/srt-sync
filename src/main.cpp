#include <iostream>
#include <srt.h>
#include <string>
#include <srt-sync/Thread.hpp>

int main() {
	srt_startup();
	srt_setloglevel(LOG_DEBUG);
	srt_sync::ServerThread thread;
	thread.setup();
	thread.start();

	while (1) {
		std::cout << "> " << std::endl;
		std::string cmd;
		std::cin >> cmd;
		if (cmd == "stop") {
			thread.stop();
			break;
		} else if (cmd == "probe") {
			thread.decoder.probe_input();
		}
	}
	srt_cleanup();
	return 0;
}
