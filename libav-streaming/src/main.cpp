#include <iostream>
#include <stdlib.h>
#include "streamer.h"

int main(int argc, char *argv[])
{
        const char* src = getenv("SRC");
        const char* rtmp = getenv("RTMP");
        const char* rtsp = getenv("RTSP");
        if (!src || !rtmp || !rtsp) return 0;
	std::cout << std::endl
		<< "Welcome to RTMP streamer" << std::endl
		<< "Written by Jiewen Wang <https://jiewen.wang>" << std::endl
		<< std::endl;
        std::cout << "Now streaming from " << getenv("SRC") << std::endl;
	Streamer streamer(src, rtmp, rtsp);

	return streamer.Stream();
}
