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
			while (1) {
				auto frame = thread.decoder.get_source()->pull();
				if (frame)
					std::cout << "Video time: " << (*frame).video_frame->pts <<
						" Audio time: " << (*frame).audio_frame->pts << std::endl;
			}
		}
	}
	srt_cleanup();
	return 0;
}
