clean:
	rm -rf ./build && mkdir build

build: clean
	docker run -w /files --rm -it  -v `pwd`:/files libav-streaming \
	g++ -std=c++11 -L/files/src -L/opt/ffmpeg/lib -I/opt/ffmpeg/include/ /files/src/streamer.cpp /files/src/main.cpp \
	  -lavcodec -lavformat -lavfilter -lavdevice -lswresample -lswscale -lavutil \
	  -o /files/build/streamer

run: build
	docker run -e SRC="https://m-test-public.s3-us-west-2.amazonaws.com/challenge/example.webm" \
	  -e RTMP="rtmp://127.0.0.1/live/example" -e RTSP="rtsp://127.0.0.1:8554/example" \
	  -w /files --rm --network host -it -v `pwd`:/files libav-streaming /files/build/streamer 
