#include <iostream>
#include <stdlib.h>
#include "streamer.h"

int main(int argc, char *argv[])
{
	std::cout << std::endl
		<< "Welcome to RTMP streamer" << std::endl
		<< "Written by Jiewen Wang <https://jiewen.wang>" << std::endl
		<< std::endl;
        std::cout << "Now streaming from " << getenv("SRC") << std::endl;
	//Streamer streamer("samples/test.avi", "rtmp://127.0.0.1/live");
	Streamer streamer(getenv("SRC"), getenv("DST"));

	return streamer.Stream();
}
