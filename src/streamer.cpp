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
    sws_freeContext(sws_ctx);
}

int Streamer::setupInput(const char *_videoFileName)
{
    if ((ret = avformat_open_input(&ifmt_ctx, _videoFileName, NULL, NULL)) < 0)
    {
        cout << "Could not open input file." << endl;
        return -1;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0)
    {
        cout << "Failed to retrieve input stream information" << endl;
        return -1;
    }

    for (int i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoIndex = i;
            break;
        }
    }

    av_dump_format(ifmt_ctx, NULL, _videoFileName, NULL);
    return 0;
}

int Streamer::setupOutput(const char *_rtmpServerAdress)
{
    avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", _rtmpServerAdress); //RTMP
    if (!ofmt_ctx)
    {
        cout << "Could not create output context" << endl;
        ret = AVERROR_UNKNOWN;
        return -1;
    }


    ofmt = ofmt_ctx->oformat;
    ofmt->video_codec = AV_CODEC_ID_H264;
    for (int i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        if (i != videoIndex) continue;
        
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream)
        {
            cout << "Failed allocating output stream" << endl;
            ret = AVERROR_UNKNOWN;
            return -1;
        }

        dec_ctx = in_stream->codec;
        enc_ctx = out_stream->codec;

        enc_ctx->codec_id = AV_CODEC_ID_H264;
        enc_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
        enc_ctx->qmin = 1;
        enc_ctx->qmax = 41;
        enc_ctx->qcompress = 0.6;
        enc_ctx->bit_rate = 500*1000;

        decoder = avcodec_find_decoder(dec_ctx->codec_id);
        if (!decoder) {
            cout << "Decoder not found " << endl;
            return -1;
        }

        ret = avcodec_parameters_to_context(dec_ctx, in_stream->codecpar);

        //encoder = avcodec_find_encoder(dec_ctx->codec_id);//AV_CODEC_ID_H264);
        encoder = avcodec_find_encoder_by_name("libx264");
        if (!encoder) {
            cout << "Encoder not found " << endl;
            return -1;
        }

        // set output properties

        src_w = dec_ctx->width;
        src_h = dec_ctx->height;

        // get output resolution
        dst_h = dst_w * src_h / src_w;

        enc_ctx->width = dst_w;
        enc_ctx->height = dst_h;
        enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
        enc_ctx->pix_fmt = dst_pix_fmt;

        AVRational time_base = {1, dst_fps};
        enc_ctx->time_base = time_base;

        // enc_ctx->codec_tag = 0;
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
             enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);

        ret = avcodec_open2(dec_ctx, decoder, NULL);
        if (ret < 0) {
            cout << "Failed to open decoder" << endl;
            return ret;
        }

        AVDictionary *opts = NULL;
        av_dict_set(&opts, "preset", "ultrafast", 0);
        av_dict_set(&opts, "x264-params", "keyint=60:min-keyint=60:scenecut=0:force-cfr=1", 0);

        ret = avcodec_open2(enc_ctx, encoder, &opts);
        if (ret < 0) {
            cout << "Failed to open encoder" << endl;
            return ret;
        }
    }
    av_dump_format(ofmt_ctx, 0, _rtmpServerAdress, 1);
    return 0;
}

int Streamer::setupScaling()
{
    src_pix_fmt = dec_ctx->pix_fmt;

    // create scaling context
    sws_ctx = sws_getContext(src_w, src_h, src_pix_fmt,
                             dst_w, dst_h, dst_pix_fmt,
                             SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx) {
        cout << "Could not create scaling context." << endl;
        return -1;
    }


    return 0;
}

int Streamer::encodeVideo(AVFrame *input_frame) {

    ret = avcodec_send_frame(enc_ctx, input_frame);

    AVStream * in_stream = ifmt_ctx->streams[videoIndex];
    AVStream * out_stream = ofmt_ctx->streams[videoIndex];

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, &output_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            cout << "Error while receiving packet from encoder: " << ret << endl;
            return -1;
        }

        output_packet.stream_index = videoIndex;
        //output_packet->duration = out_stream->time_base.den / out_stream->time_base.num / 
        //    in_stream->avg_frame_rate.num * in_stream->avg_frame_rate.den;
        output_packet.pts = outIndex++;//pkt.pts;
        output_packet.dts = output_packet.pts;//pkt.dts;
        output_packet.duration = 1;//pkt.duration;
        AVRational dst_time_base = {1, dst_fps};
        av_packet_rescale_ts(output_packet, dst_time_base, out_stream.time_base);
        ret = av_interleaved_write_frame(ofmt_ctx, output_packet);
        if (ret < 0) { cout << "Error while writing packet"  << ret << endl; return ret;}
    }
    return 0;
}

int Streamer::Stream()
{
    // Open output URL
    ret = avio_open(&ofmt_ctx->pb, rtmpServerAdress, AVIO_FLAG_WRITE);
    if (ret < 0)
    {
        cout << "Could not open output URL " << rtmpServerAdress << endl;
        return -1;
    }

    AVFrame *frame = av_frame_alloc();
    AVFrame *frame2 = av_frame_alloc();

    int num_bytes = avpicture_get_size(dst_pix_fmt, dst_w, dst_h);
    uint8_t* frame2_buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
    avpicture_fill((AVPicture*)frame2, frame2_buffer, dst_pix_fmt, dst_w, dst_h);
    frame2->width = dst_w;
    frame2->height = dst_h;
    frame2->format = dst_pix_fmt;

    AVStream *in_stream = ifmt_ctx->streams[videoIndex];
    AVStream *out_stream = ofmt_ctx->streams[videoIndex];
    AVRational time_base = in_stream->time_base;
    AVRational dst_time_base = {1, dst_fps};
    AVRational time_base_q = {1, AV_TIME_BASE};
    AVRational dst_frame_rate = {dst_fps, 1};
    out_stream->time_base = dst_time_base;
    out_stream->avg_frame_rate = dst_frame_rate;
    out_stream->r_frame_rate = dst_frame_rate;
    avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    out_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;

    // Write file header
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0)
    {
        cout << "Error occurred when opening output URL" << endl;
        return -1;
    }

    startTime = av_gettime();
    while (true)
    {
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0) {
            avio_seek(ifmt_ctx->pb, 0, SEEK_SET);
	    avformat_seek_file(ifmt_ctx, videoIndex, 0, 0, in_stream->duration, 0);
            cout << "loop stream" << endl;
            continue;
        }
	    if (pkt.stream_index != videoIndex) continue;

        if (pkt.pts == AV_NOPTS_VALUE)
        {
            AVRational time_base1 = in_stream->time_base;
            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
            pkt.pts = (double)(frameIndex * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
            pkt.dts = pkt.pts;
            pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
        }
        int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
        int64_t now_time = av_gettime() - startTime;
        if (pts_time > now_time) av_usleep(pts_time - now_time);
        // pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        // pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        // pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        //pkt.pos = -1;
        frameIndex++;
        ret = avcodec_send_packet(dec_ctx, &pkt);
        if (ret < 0) {
            cout << "Error while sending packet to decoder:" << ret << endl;
            return ret;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(dec_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            else if (ret < 0) {
                cout << "Error while receiving frame from decoder:" << ret << endl;
                return ret;
            }
            ret = sws_scale(sws_ctx, frame->data, frame->linesize, 
                        0, src_h, frame2->data, frame2->linesize);

            if (ret < 0) {
                cout << "Error while scaling a frame:" << ret << endl;
                return ret;
            }

            ret = encodeVideo(frame2);
            if (ret < 0) {cout << "Error while encoding\n"; return -1; };
        }
        
        // if (pkt.pts == AV_NOPTS_VALUE)
        // {
        //     //Write PTS
        //     AVRational time_base1 = ifmt_ctx->streams[videoIndex]->time_base;
        //     //Duration between 2 frames (us)
        //     int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(ifmt_ctx->streams[videoIndex]->r_frame_rate);
        //     //Parameters
        //     pkt.pts = (double)(frameIndex * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
        //     pkt.dts = pkt.pts;
        //     pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
        // }

        // if (pkt.stream_index == videoIndex)
        // {
        //     AVRational time_base = ifmt_ctx->streams[videoIndex]->time_base;
        //     AVRational time_base_q = {1, AV_TIME_BASE};
        //     int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
        //     int64_t now_time = av_gettime() - startTime;
        //     if (pts_time > now_time)
        //         av_usleep(pts_time - now_time);
        // }

        // in_stream = ifmt_ctx->streams[pkt.stream_index];
        // out_stream = ofmt_ctx->streams[pkt.stream_index];
        // pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        // pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        // pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        // pkt.pos = -1;
        // if (pkt.stream_index == videoIndex)
        // {
        //     frameIndex++;
        // }
        // //ret = av_write_frame(ofmt_ctx, &pkt);
        // ret = av_interleaved_write_frame(ofmt_ctx, &pkt);

        // av_free_packet(&pkt);
    }

    cout << ret << endl;
    //Write file trailer
    av_write_trailer(ofmt_ctx);
    avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    return 0;
}
