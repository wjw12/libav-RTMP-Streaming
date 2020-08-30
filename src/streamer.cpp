#include "streamer.h"

using namespace std;

Streamer::Streamer(const char *_videoFileName,
                   const char *_rtmpServerAdress) : videoFileName(_videoFileName),
                                                    rtmpServerAdress(_rtmpServerAdress)
{
    av_register_all();
    avformat_network_init();

    ret = setupInput(videoFileName);
    if (ret < 0) return;

    ret = setupOutput(rtmpServerAdress);
    if (ret < 0) return;

    ret = setupScaling();
    if (ret < 0) return;
}

Streamer::~Streamer()
{
    // av_freep(&src_data[0]);
    // av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);
}

int Streamer::setupInput(const char *_videoFileName)
{
    if ((ret = avformat_open_input(&ifmt_ctx, _videoFileName, NULL, NULL)) < 0)
    {
        cerr << "Could not open input file." << endl;
        return -1;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0)
    {
        cerr << "Failed to retrieve input stream information" << endl;
        return -1;
    }

    for (int i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoIndex = i;
            // AVCodecParameters *codec_params = ifmt_ctx->streams[i]->codecpar;
            // AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
            break;
        }
    }

    av_dump_format(ifmt_ctx, NULL, _videoFileName, NULL);
    return 0;
}

int Streamer::setupOutput(const char *_rtmpServerAdress)
{
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, _rtmpServerAdress); //RTMP
    if (!ofmt_ctx)
    {
        cerr << "Could not create output context" << endl;
        ret = AVERROR_UNKNOWN;
        return -1;
    }

    ofmt = ofmt_ctx->oformat;
    for (int i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        if (i != videoIndex) continue;
        
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
        if (!out_stream)
        {
            cerr << "Failed allocating output stream" << endl;
            ret = AVERROR_UNKNOWN;
            return -1;
        }

        icodec_ctx = in_stream->codec;
        ocodec_ctx = out_stream->codec;
        ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
        if (ret < 0)
        {
            cerr << "Failed to copy context from input to output stream codec context" << endl;
            return -1;
        }

        // set output properties
        src_w = icodec_ctx->width;
        src_h = icodec_ctx->height;

        // get output resolution
        dst_h = dst_w * src_h / src_w;

        ocodec_ctx->pix_fmt = dst_pix_fmt;

        // sample rate TODO

        ocodec_ctx->codec_tag = 0;

        if (ofmt_ctx->oformat->flags & 1 << 22)
            ocodec_ctx->flags |= 1 << 22;
    }
    return 0;
}

int Streamer::setupScaling()
{
    src_pix_fmt = icodec_ctx->pix_fmt;

    // create scaling context
    sws_ctx = sws_getContext(src_w, src_h, src_pix_fmt,
                             dst_w, dst_h, dst_pix_fmt,
                             SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx) {
        cerr << "Could not create scaling context." << endl;
        return -1;
    }

    // allocate source and dest image buffers
    // if ((ret = av_image_alloc(src_data, src_linesize, 
    //                     src_w, src_h, src_pix_fmt, 0)) < 0) {
    //     cerr << "Could not allocate source image" << endl;
    //     return -1;
    // }
    
    // if ((ret = av_image_alloc(dst_data, dst_linesize,
    //                     dst_w, dst_h, dst_pix_fmt, 1)) < 0) {
    //     cerr << "Could not allocate destination image" << endl;
    //     return -1;
    // }

    // dst_bufsize = ret;


    return 0;
}

int Streamer::Stream()
{
    av_dump_format(ofmt_ctx, 0, rtmpServerAdress, 1);
    //Open output URL
    if (!(ofmt->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&ofmt_ctx->pb, rtmpServerAdress, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            cerr << "Could not open output URL '%s'" << rtmpServerAdress << endl;
            return -1;
        }
    }
    //Write file header
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0)
    {
        cerr << "Error occurred when opening output URL" << endl;
        return -1;
    }

    
    AVFrame *frame = av_frame_alloc();
    AVFrame *frame2 = av_frame_alloc();

    startTime = av_gettime();
    while (1)
    {
        AVStream *in_stream, *out_stream;
        ret = av_read_frame(ifmt_ctx, pkt);
        if (ret < 0) {
            cout << "end of stream" << endl;
            break;
        }

        ret = avcodec_send_packet(icodec_ctx, pkt);
        if (ret < 0) {
            cerr << "send packet error" << endl;
            break;
        }

        ret = avcodec_receive_frame(icodec_ctx, frame)
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        sws_scale(sws_ctx, frame->data, frame->linesize, 0, src_h, frame2->data, frame2->linesize);
        avcodec_send_frame(ocodec_ctx, frame2);

        avcodec_receive_packet(ocodec_ctx, pkt);

        if (pkt->pts == AV_NOPTS_VALUE)
        {
            //Write PTS
            AVRational time_base1 = ifmt_ctx->streams[videoIndex]->time_base;
            //Duration between 2 frames (us)
            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(ifmt_ctx->streams[videoIndex]->r_frame_rate);
            //Parameters
            pkt->pts = (double)(frameIndex * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
            pkt->dts = pkt->pts;
            pkt->duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
        }

        if (pkt->stream_index == videoIndex)
        {
            AVRational time_base = ifmt_ctx->streams[videoIndex]->time_base;
            AVRational time_base_q = {1, AV_TIME_BASE};
            int64_t pts_time = av_rescale_q(pkt->dts, time_base, time_base_q);
            int64_t now_time = av_gettime() - startTime;
            if (pts_time > now_time)
                av_usleep(pts_time - now_time);
        }

        in_stream = ifmt_ctx->streams[pkt->stream_index];
        out_stream = ofmt_ctx->streams[pkt->stream_index];
        pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
        pkt->pos = -1;
        if (pkt->stream_index == videoIndex)
        {
            frameIndex++;
        }
        //ret = av_write_frame(ofmt_ctx, &pkt);
        ret = av_interleaved_write_frame(ofmt_ctx, pkt);

        if (ret < 0)
        {
            cerr << "Error muxing packet" << endl;
            break;
        }

        av_free_packet(pkt);
    }

    //Write file trailer
    av_write_trailer(ofmt_ctx);
    avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    return 0;
}