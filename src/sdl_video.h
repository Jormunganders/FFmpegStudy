//
// Created by yunrui on 2019/11/25.
//

#ifndef FFMPEG_SDL_VIDEO_H
#define FFMPEG_SDL_VIDEO_H

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 360

namespace sdl_video {

#include <iostream>

    extern "C" {

#include <libavformat/avformat.h>
#include <SDL2/SDL.h>
#include <libswscale/swscale.h>

#include <libavutil/imgutils.h>

    }
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
    AVFrame *YUVFrame;
    SwsContext *swsContext;

    int videoIndex = -1;

    void drawFrame(AVFrame *frame);

    void waitKey();

    void ffmpegLogFunc(void *ptr, int level, const char *fmt, va_list vl);

    void initSDL2() {
        if (SDL_Init(SDL_INIT_VIDEO |
                     SDL_INIT_EVENTS |
                     SDL_INIT_TIMER)) {
            return;
        }
        window = SDL_CreateWindow("Player",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  WINDOW_WIDTH,
                                  WINDOW_HEIGHT,
                                  SDL_WINDOW_OPENGL);
        if (!window) {
            return;
        }
        renderer = SDL_CreateRenderer(window, -1, 0);
//      SDL_PIXELFORMAT_NV12
//      SDL_PIXELFORMAT_YV12
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_NV12,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    WINDOW_WIDTH, WINDOW_HEIGHT);
        rect.x = 0;
        rect.y = 0;
        rect.w = WINDOW_WIDTH;
        rect.h = WINDOW_HEIGHT;
    }

    void prepareDecoder(const char *url) {
        avformat_network_init();
        av_log_set_callback(ffmpegLogFunc);

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
        videoIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO,
                                         -1, -1, NULL, 0);
        retCode = avcodec_parameters_to_context(codecContext,
                                                formatContext->streams[videoIndex]->codecpar);
        if (retCode != 0) {
            return;
        }

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
        YUVFrame = av_frame_alloc();

        int outBufferSize =
                av_image_get_buffer_size(AV_PIX_FMT_NV12,
                                         codecContext->width,
                                         codecContext->height,
                                         1);
        auto *outBuffer = (uint8_t *) av_malloc(
                outBufferSize * sizeof(uint8_t));
        // 这个 buffer 是用于存缓存数据的
        av_image_fill_arrays(YUVFrame->data, YUVFrame->linesize,
                             outBuffer,
                             AV_PIX_FMT_NV12,
                             codecContext->width, codecContext->height, 1);


        swsContext = sws_getContext(
                codecContext->width, codecContext->height, codecContext->pix_fmt,
                codecContext->width, codecContext->height, AV_PIX_FMT_NV12,
                SWS_BILINEAR, nullptr, nullptr, nullptr
        );

    }

    void decodeFrame() {
        int sendCode = 0;
        // Return the next frame of a stream.
        while (av_read_frame(formatContext, packet) == 0) {
            if (packet->stream_index == videoIndex) {

                // Return decoded output data from a decoder.
                // 接受解码前的包数据
                while (avcodec_receive_frame(codecContext, frame) == 0) {
                    drawFrame(frame);
                }

                // Supply raw packet data as input to a decoder.
                // 发送解码前的包数据
                sendCode = avcodec_send_packet(codecContext, packet);
                if (sendCode == 0) {
                    cout << "[debug] " << "SUCCESS" << endl;
                } else if (sendCode == AVERROR_EOF) {
                    cout << "[debug] " << "EOF" << endl;
                } else if (sendCode == AVERROR(EAGAIN)) {   //  当前发送/接受队里已满/已空，需要调用对应的 recive/send
                    cout << "[debug] " << "EAGAIN" << endl;
                } else {
                    cout << "[debug] " << av_err2str(AVERROR(sendCode)) << endl;
                }
            }

            av_packet_unref(packet);

            // 事件处理
            SDL_Event event;
            SDL_PollEvent(&event);
            switch (event.type) {
                case SDL_QUIT:
                    /*case SDL_KEYDOWN:
                    case SDL_MOUSEBUTTONDOWN:*/
                    return;
            }
        }
    }

    void drawFrame(AVFrame *frame) {
        if (frame == nullptr)
            return;
        sws_scale(swsContext, frame->data, frame->linesize,
                  0, codecContext->height,
                  YUVFrame->data, YUVFrame->linesize);
        cout << YUVFrame->format << endl;
        int retCode = SDL_UpdateYUVTexture(texture, &rect,
                                           YUVFrame->data[0], YUVFrame->linesize[0],
                                           YUVFrame->data[1], YUVFrame->linesize[1],
                                           YUVFrame->data[2], YUVFrame->linesize[2]
        );
        /*int retCode = SDL_UpdateYUVTexture(texture, &rect,
                                           frame->data[0], frame->linesize[0],
                                           frame->data[1], frame->linesize[1],
                                           frame->data[2], frame->linesize[2]
        );*/
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
        SDL_RenderPresent(renderer);
        SDL_Delay(40);
    }

    void free() {
        if (formatContext != nullptr) avformat_close_input(&formatContext);
        if (codecContext != nullptr) avcodec_free_context(&codecContext);
        if (packet != nullptr) av_packet_free(&packet);
        if (frame != nullptr) av_frame_free(&frame);
        sws_freeContext(swsContext);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void playVideo(const char *url) {
        initSDL2();
        prepareDecoder(url);
        decodeFrame();
        free();
    }

    void waitKey() {
        SDL_Event e;
        bool quit = false;
        while (!quit) {
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    quit = true;
                }
                if (e.type == SDL_KEYDOWN) {
                    quit = true;
                }
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    quit = true;
                }
            }
        }
    }

//    void*, int, const char*, va_list
    void ffmpegLogFunc(void *ptr, int level, const char *fmt, va_list vl) {
        FILE *fp = fopen("FFmpegLog.txt", "a+");
        if (fp) {
            vfprintf(fp, fmt, vl);
            fflush(fp);
            fclose(fp);
        }
    }

}
#endif //FFMPEG_SDL_VIDEO_H
