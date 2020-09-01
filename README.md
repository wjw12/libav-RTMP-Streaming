# 📽 Streaming Service: stream.jiewen.wang

## System Overview
![system](https://storage.googleapis.com/jiewen-storage/system.png)

The system consists of 4 dockerized services.

1. REST API Service built with Node.js+Express.js 
2. Video Streaming and Transcoding Service built with C++ and libav
3. RTMP Server using [nginx-rtmp](https://github.com/tiangolo/nginx-rtmp-docker)
4. RTSP Server using [rtsp-simple-server](https://github.com/aler9/rtsp-simple-server)

The system allows users to specify a static video/image source URL and serves it as an infinite live stream via RTMP and RTSP protocal.

The REST API service listens to port 80 and responds to user calls. When it receives a POST call with a valid video/image source URL and name, it spawns a new docker process to ingest the media and generate a live stream.

The streaming service is written in C++ using libav, which is the backbone of ffmpeg, to transcode, scale and mux the input media. Libav is capable of manipulating a wide range of video/audio formats and media protocols. Theoretically what ffmpeg is capable of can be implemented with libav.

Two open source solutions for RTMP and RTSP servers are used. Both are containerized and ready-to-use in any production environment. These services allow publishing a video stream of any formats (strictly speaking, a majority of popular formats) to a specific endpoint, and user can view the live stream in VLC or any RTMP/RTSP client. 

The RTSP server can run in either UDP+TCP or TCP-only mode. For ease of programming, the server is configured to use TCP only through port 8554.

## Installation
Before start, we need to set the `HOST_STREAMING` environment variable in `docker-compose.yml` to the local host name. Here I've set it to be `streaming.jiewen.wang` on my own machine.

In a Linux machine with Docker Engine and docker-compose installed, run the following commands:

`sudo docker-compose build .`
`sudo docker-compose up`

API, RTMP and RTSP services should be up now. New docker instances will be launched when the system receives a valid POST API call.

Don't forget to allow port 80, 1935, 8554 through the firewall.

## Repository Structure
📦streaming.jiewen.wang
 ┣ 📂express-api
 ┃ ┣ 📜Dockerfile
 ┃ ┣ 📜package.json
 ┃ ┗ 📜server.js
 ┣ 📂libav-streaming
 ┃ ┣ 📂src
 ┃ ┃ ┣ 📜main.cpp
 ┃ ┃ ┣ 📜streamer.cpp
 ┃ ┃ ┗ 📜streamer.h
 ┃ ┣ 📜Makefile
 ┃ ┣ 📜Dockerfile
 ┣ 📜docker-compose.yml
 ┣ 📜rtsp-simple-server.yml (this is RTSP server config file)

## Testing Commands
`curl -X POST -d '{"name":"example_mp4","url":"https://m-test-public.s3-us-west-2.amazonaws.com/challenge/example.mp4"}' -H "Content-Type: application/json" stream.jiewen.wang/stream`

`curl -X POST -d '{"name":"example_avi","url":"https://m-test-public.s3-us-west-2.amazonaws.com/challenge/example.avi"}' -H "Content-Type: application/json" stream.jiewen.wang/stream`

`curl -X POST -d '{"name":"example_webm","url":"https://m-test-public.s3-us-west-2.amazonaws.com/challenge/example.webm"}' -H "Content-Type: application/json" stream.jiewen.wang/stream`

`curl -X POST -d '{"name":"example_png","url":"https://m-test-public.s3-us-west-2.amazonaws.com/challenge/example.png"}' -H "Content-Type: application/json" stream.jiewen.wang/stream`

`curl -X POST -d '{"name":"example_jpg","url":"https://m-test-public.s3-us-west-2.amazonaws.com/challenge/example.jpg"}' -H "Content-Type: application/json" stream.jiewen.wang/stream`

`curl -X GET stream.jiewen.wang/stream`

`curl -X GET stream.jiewen.wang/stream/example_png`

`curl -X GET stream.jiewen.wang/stream/example_webm`

`curl -X DELETE stream.jiewen.wang/stream/example_mp4`
