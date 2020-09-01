#ifndef _STREAMER_H_
#define _STREAMER_H_

#include <iostream>
#include <chrono>
#include <vector>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
};


class Streamer
{
public:
  Streamer(const char *videoFileName,
           const char *rtmpServerAddress);
  ~Streamer();
  int Stream();


private:
  int setupInput(const char *videoFileName);
  int setupOutput(const char *rtmpServerAddress, const char *rtspServerAddress);
  int setupScaling();
  int encodeVideo(AVFrame *inputFrame);

  AVOutputFormat *ofmt = NULL;
  AVOutputFormat *ofmt2 = NULL;
  AVFormatContext *ifmt_ctx = NULL;
  AVFormatContext *ofmt_ctx = NULL; // RTMP
  AVFormatContext *ofmt_ctx2 = NULL; // RTSP
  AVCodecContext *dec_ctx = NULL;
  AVCodecContext *enc_ctx = NULL; // RTMP
  AVCodecContext *enc_ctx2 = NULL; // RTSP
  AVCodec *encoder = NULL;
  AVCodec *decoder = NULL;
  AVPacket pkt;

  int ret;

  const char *videoFileName;
  const char *rtmpServerAddress;
  const char *rtspServerAddress;

  int videoIndex = -1;
  std::vector<int> otherMediaIndices;
  int frameIndex = 0;
  int outIndex = 0;
  int64_t startTime = 0;

  enum AVPixelFormat src_pix_fmt;
  enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_YUV420P; // output pixel format
  int src_w, src_h, dst_h;
  int dst_w = 420;
  int dst_fps = 25;
  struct SwsContext *sws_ctx;
};

#endif
