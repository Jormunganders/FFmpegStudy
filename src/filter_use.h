//
// Created by yunrui on 2019/11/26.
//

#ifndef FFMPEG_FILTER_USE_H
#define FFMPEG_FILTER_USE_H

namespace filter_use {

#include <iostream>

    extern "C"
    {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <SDL2/SDL.h>

    };

    using namespace std;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Rect rect;

    AVFormatContext *formatContext;
    AVCodecContext *codecContext;
    AVCodec *codec;
    AVPacket *packet;
    AVFrame *frame;
    AVFrame *outFrame;

    AVFilterGraph *filterGraph;
    AVFilterContext *bufferSinkContext;
    AVFilterContext *bufferSrcContext;

    int videoIndex = -1;

    int width, height;

    const char *filter_description = "movie=../res/hello.bmp[wm];[in][wm]overlay=5:5[out]";

    void initSDL();

    void initFilter();

    void initDecoder(const char *url);

    void decodeFrame();

    void drawFrame(AVFrame *frame);

    void free();

    void addFilter();

    void show(const char *url) {
        initDecoder(url);
        initSDL();
        initFilter();
        decodeFrame();
        free();
    }

    void initDecoder(const char *url) {
        int retCode;
        formatContext = avformat_alloc_context();
        if (!formatContext) {
            return;
        }
        retCode = avformat_open_input(&formatContext, url, nullptr, nullptr);
        if (retCode != 0) {
            return;
        }
        retCode = avformat_find_stream_info(formatContext, nullptr);
        if (retCode != 0) {
            return;
        }
        codecContext = avcodec_alloc_context3(nullptr);
        if (!codecContext) {
            return;
        }
        videoIndex = av_find_best_stream(formatContext,
                                         AVMEDIA_TYPE_VIDEO,
                                         -1,
                                         -1,
                                         nullptr,
                                         0);
        retCode = avcodec_parameters_to_context(codecContext,
                                                formatContext->
                                                        streams[videoIndex]->codecpar);
        if (retCode != 0) {
            return;
        }
        width = codecContext->width;
        height = codecContext->height;

        codec = avcodec_find_decoder(codecContext->codec_id);
        if (codec == nullptr) {
            return;
        }
        retCode = avcodec_open2(codecContext, codec, nullptr);
        if (retCode != 0) {
            return;
        }

        packet = av_packet_alloc();
        frame = av_frame_alloc();
        outFrame = av_frame_alloc();
    }

    void initSDL() {
        if (SDL_Init(SDL_INIT_VIDEO |
                     SDL_INIT_EVENTS |
                     SDL_INIT_TIMER)) {
            return;
        }
        window = SDL_CreateWindow("Player",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  width,
                                  height,
                                  SDL_WINDOW_OPENGL);
        if (!window) {
            return;
        }
        renderer = SDL_CreateRenderer(window, -1, 0);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    width, height);
        rect.x = 0;
        rect.y = 0;
        rect.w = width;
        rect.h = height;
    }

    void decodeFrame() {
        while (av_read_frame(formatContext, packet) == 0) {
            if (packet->stream_index == videoIndex) {
                avcodec_send_packet(codecContext, packet);
                while (avcodec_receive_frame(codecContext, frame) == 0) {
//                    addFilter();
                    drawFrame(frame);
                }
            }
            av_packet_unref(packet);

            SDL_Event event;
            SDL_PollEvent(&event);
            switch (event.type) {
                case SDL_QUIT:
                    return;
            }
        }
    }

    void drawFrame(AVFrame *frame) {
        if (frame == nullptr)
            return;
        SDL_UpdateYUVTexture(texture, &rect,
                             frame->data[0], frame->linesize[0],
                             frame->data[1], frame->linesize[1],
                             frame->data[2], frame->linesize[2]);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
        SDL_RenderPresent(renderer);
        SDL_Delay(33);
    }

    void free() {
        if (formatContext != nullptr) avformat_close_input(&formatContext);
        if (codecContext != nullptr) avcodec_free_context(&codecContext);
        if (packet != nullptr) av_packet_free(&packet);
        if (frame != nullptr) av_frame_free(&frame);
        if (outFrame != nullptr) av_frame_free(&outFrame);
        avfilter_graph_free(&filterGraph);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void initFilter() {
        char args[512];
        int retCode;
        const AVFilter *bufferSrc =
                avfilter_get_by_name("buffer");
        const AVFilter *bufferSink =
                avfilter_get_by_name("ffbuffersink");
        AVFilterInOut *outputs = avfilter_inout_alloc();
        AVFilterInOut *inputs = avfilter_inout_alloc();
        AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P,
                                    AV_PIX_FMT_NONE};
        AVBufferSinkParams *bufferSinkParams;

        filterGraph = avfilter_graph_alloc();
        retCode = avfilter_graph_create_filter(&bufferSrcContext,
                                               bufferSrc,
                                               "in",
                                               args,
                                               NULL,
                                               filterGraph);
        if (retCode < 0) {
            return;
        }
        outputs->name = av_strdup("in");
        outputs->filter_ctx = bufferSrcContext;
        outputs->pad_idx = 0;
        outputs->next = NULL;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = bufferSinkContext;
        inputs->pad_idx = 0;
        inputs->next = NULL;
        if (avfilter_graph_parse_ptr(filterGraph,
                                     filter_description,
                                     &inputs,
                                     &outputs,
                                     NULL) < 0) {
            cout << "Error，不匹配" << endl;
            return;
        }
        if (avfilter_graph_config(filterGraph, NULL) < 0) {
            return;
        }
    }

    void addFilter() {
        if (av_buffersrc_add_frame(bufferSrcContext, frame) < 0) {
            cout << "Error while feeding the filtergraph" << endl;
            return;
        }
        int retCode;
        while (true) {
            retCode = av_buffersink_get_frame(bufferSinkContext,
                                              outFrame);
            if (retCode < 0) return;
        }
    }

}

#endif //FFMPEG_FILTER_USE_H
