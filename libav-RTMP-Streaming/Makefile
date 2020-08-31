clean:
	rm -rf ./build/*

build: clean
	docker run -w /files --rm -it  -v `pwd`:/files leandromoreira/ffmpeg-devel \
	g++ -std=c++11 -L/files/src -L/opt/ffmpeg/lib -I/opt/ffmpeg/include/ /files/src/streamer.cpp /files/src/main.cpp \
	  -lavcodec -lavformat -lavfilter -lavdevice -lswresample -lswscale -lavutil -lzmq \
	  -o /files/build/streamer

run: build
	docker run -e SRC="https://m-test-public.s3-us-west-2.amazonaws.com/challenge/example.webm" -e DST="rtmp://localhost/live/example" -w /files --rm --network host -it -v `pwd`:/files leandromoreira/ffmpeg-devel /files/build/streamer 
