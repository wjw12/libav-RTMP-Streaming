#include <iostream>

#include "streamer.h"

int main(int argc, char *argv[])
{
	std::cout << std::endl
		<< "Welcome to RTMP streamer 📽" << std::endl
		<< "Written by @juniorxsound <https://orfleisher.com>" << std::endl
		<< std::endl;

	Streamer streamer("samples/test.mp4", "rtmp://127.0.0.1/live");

	return streamer.Stream();
}
