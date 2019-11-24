//
// Created by yunrui on 2019/11/21.
//

#ifndef FFMPEG_AVFORMAT_USE_H
#define FFMPEG_AVFORMAT_USE_H

#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
};

using namespace std;

/**
 * avformat 的简单使用
 *
 * 分离视频
 *
 * @param url
 */
void separate_video(const char *url) {
    FILE *output = fopen("./output.h264", "wb+");
    int ret_code;
    int media_index = -1;
    // 分配一个存储解封装后的 Packet
    AVPacket *packet = av_packet_alloc();

    avformat_network_init();

    // 创建 H264_mp4 的 filter
    const AVBitStreamFilter *bsf =
            av_bsf_get_by_name("h264_mp4toannexb");
    AVBSFContext *ctx = NULL;

    AVFormatContext *avFormatContext = avformat_alloc_context();
    if (avFormatContext == nullptr) {
        cout << "[error] AVFormat alloc error" << endl;
        goto failed;
    }

    ret_code = avformat_open_input(&avFormatContext, url, nullptr, nullptr);
    if (ret_code < 0) {
        cout << "[error] open input failed " <<
             av_err2str(AVERROR(ret_code)) << endl;
        goto failed;
    }

    ret_code = avformat_find_stream_info(avFormatContext, nullptr);
    if (ret_code < 0) {
        cout << "[error] find stream info failed " <<
             av_err2str(AVERROR(ret_code)) << endl;
        goto failed;
    }

    media_index = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_VIDEO,
                                      -1, -1, NULL, 0);
    if (media_index < 0) {
        cout << "[error] find stream index error" << endl;
        goto failed;
    }

    // alloc bsf
    ret_code = av_bsf_alloc(bsf, &ctx);
    if (ret_code < 0) {
        cout << "[error] BSF alloc failed "
             << av_err2str(AVERROR(ret_code)) << endl;
        goto failed;
    }

    // 复制解码器参数到 BSFContext
    ret_code = avcodec_parameters_copy(ctx->par_in,
                                       avFormatContext->streams[media_index]->codecpar);
    if (ret_code < 0) {
        cout << "[error] BSF copy parameter failed "
             << av_err2str(AVERROR(ret_code)) << endl;
        goto failed;
    }

    // 同步 time_base
    ctx->time_base_in = avFormatContext->streams[media_index]->time_base;

    // 初始化 bsf
    ret_code = av_bsf_init(ctx);
    if (ret_code < 0) {
        cout << "[error] BSF init failed "
             << av_err2str(AVERROR(ret_code)) << endl;
        goto failed;
    }


    while (av_read_frame(avFormatContext, packet) == 0) {
        if (packet->stream_index != media_index) continue;

        // 发送 packet 到 BitStreamFilter
        ret_code = av_bsf_send_packet(ctx, packet);
        if (ret_code < 0) {
            cout << "[error] BSF send packet failed "
                 << av_err2str(AVERROR(ret_code)) << endl;
            goto failed;
        }

        // 接受添加 sps pps 头的 packet
        while ((ret_code = av_bsf_receive_packet(ctx, packet)) == 0) {
            // 写入到文件
            fwrite(packet->data, 1, packet->size, output);
            av_packet_unref(packet);
        }

        // 需要输入数据
        if (ret_code == AVERROR(EAGAIN)) {
            cout << "[debug] BSF EAGAIN" << endl;
            av_packet_unref(packet);
            continue;
        }

        if (ret_code == AVERROR_EOF) {
            cout << "[debug] BSF EOF " << endl;
            break;
        }

        if (ret_code < 0) {
            cout << "[error] BSF receive packet failed " <<
                 av_err2str(AVERROR(ret_code)) << endl;
            goto failed;
        }
    }

    // Flush
    ret_code = av_bsf_send_packet(ctx, nullptr);
    if (ret_code < 0) {
        cout << "[error] BSF flush send packet failed "
             << av_err2str(AVERROR(ret_code)) << endl;
        goto failed;
    }

    while ((ret_code = av_bsf_receive_packet(ctx, packet)) == 0) {
        fwrite(packet->data, 1, packet->size, output);
    }

    if (ret_code != AVERROR_EOF) {
        cout << "[debug] BSF flush EOF " << endl;
        goto failed;
    }

    failed:
    av_packet_free(&packet);
    avformat_close_input(&avFormatContext);
    avformat_network_deinit();
    av_bsf_free(&ctx);
    fclose(output);
}

#endif //FFMPEG_AVFORMAT_USE_H

/*
 * Note:
 * 如果想要播放 H264 文件的话，每个 NALU 都会有头信息以及 SPS、PPS 信息才能播放。
 * 所以，每个 packet 都需要处理一下，需要用到 AVBitStreamFilter。
 */
