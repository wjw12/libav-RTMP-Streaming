FROM leandromoreira/ffmpeg-devel
COPY . /files
RUN mkdir /files/build
RUN g++ -std=c++11 -L/files/src -L/opt/ffmpeg/lib -I/opt/ffmpeg/include/ /files/src/streamer.cpp /files/src/main.cpp \
	-lavcodec -lavformat -lavfilter -lavdevice -lswresample -lswscale -lavutil \
	-o /files/build/streamer
CMD /files/build/streamer
