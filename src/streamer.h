#ifndef _STREAMER_H_
#define _STREAMER_H_

#include <iostream>
#include <chrono>

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
           const char *rtmpServerAdress);
  ~Streamer();
  int Stream();

  AVOutputFormat *ofmt = NULL;
  AVFormatContext *ifmt_ctx = NULL;
  AVFormatContext *ofmt_ctx = NULL;
  AVCodecContext *dec_ctx = NULL;
  AVCodecContext *enc_ctx = NULL;
  AVCodec *encoder = NULL;
  AVCodec *decoder = NULL;
  AVPacket *pkt;

private:
  int setupInput(const char *videoFileName);
  int setupOutput(const char *rtmpServerAdress);
  int setupScaling();

  int ret;

  // Input file and RTMP server address
  const char *videoFileName;
  const char *rtmpServerAdress;

  int videoIndex = -1;
  int frameIndex = 0;
  int64_t startTime = 0;

  enum AVPixelFormat src_pix_fmt;
  enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_YUV420P; // output pixel format
  int src_w, src_h, dst_h;
  int dst_w = 420;
  int dst_sample_rate = 25;
  struct SwsContext *sws_ctx;

  // uint8_t *src_data[4], *dst_data[4];
  // int src_linesize[4], dst_linesize[4];
  // int dst_bufsize;
};

#endif