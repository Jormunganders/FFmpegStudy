//
// Created by yunrui on 2019/11/22.
//

namespace sdl_audio {

#include <iostream>


    extern "C" {

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>

    }

    using namespace std;

    /**
     * 准备 ffmpeg
     * @param url
     */
    void prepareFFmpeg(const char *url);

    void free();

#define MAX_AUDIO_FRAME_SIZE 192000 // 1s of 48hz 32bit audio 48000 * (32 /8)

    unsigned int audioLen = 0;  // 一帧 PCM 的数据长度
    unsigned char *audioChunk = nullptr;
    unsigned char *audioPos = nullptr;  // 当前读取的位置

    /**
     * 被 SDL 调用的回调函数，当需要获取数据喂入硬件播放时调用
     * @param codecContext
     * @param stream
     * @param len
     */
    void fill_audio(void *codecContext, Uint8 *stream, int len) {
        // SDL2 中必须首先使用 SDL_memset() 将 stream 中的数据设置为 0
        SDL_memset(stream, 0, len);
        if (audioLen == 0) {
            return;
        }

        len = len > audioLen ? audioLen : len;
        // 将数据合并到 stream 里
        SDL_MixAudio(stream, audioPos, len, SDL_MIX_MAXVOLUME);
        // 一帧的数据控制
        audioPos += len;
        audioLen -= len;
    }

    // 自己想要输出的音频格式
    SDL_AudioSpec wantSpec;
    // 重采样上下文
    SwrContext *auConvertContext;
    AVFormatContext *formatContext;
    AVCodecContext *codecContext;
    AVCodec *codec;
    AVPacket *packet;
    AVFrame *frame;
    int audioIndex = -1;
    uint64_t out_chn_layout = AV_CH_LAYOUT_STEREO;  // 输出的通道布局，双通道
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16; // 输出的声音格式
    int out_sample_rate = 44100;    // 输出的采样率
    int out_nb_samples = -1;    // 输出的音频采样
    int out_channels = -1;  // 输出的通道数
    int out_buffer_size = -1;   // 输出 buffer 大小
    unsigned char *out_buffer = NULL;   // 输出的 Buffer 数据
    uint64_t in_chn_layout = -1;    // 输入的通道布局

    void playAudio(const char *url) {
        prepareFFmpeg(url);
        out_nb_samples = codecContext->frame_size;  // 单个通道中的采样数
        out_channels = av_get_channel_layout_nb_channels(out_chn_layout);   // 输出的声道数
        in_chn_layout = av_get_default_channel_layout(codecContext->channels);  // 输出音频的布局

        // 计算输出的 buffer 的大小
        out_buffer_size = av_samples_get_buffer_size(NULL,
                                                     out_channels, out_nb_samples, out_sample_fmt, 1);

        out_buffer = static_cast<unsigned char *>(av_malloc(MAX_AUDIO_FRAME_SIZE * 2)); // 双声道

        //初始化SDL中自己想设置的参数
        wantSpec.freq = out_sample_rate;
        wantSpec.format = AUDIO_S16SYS;
        wantSpec.channels = out_channels;
        wantSpec.silence = 0;
        wantSpec.samples = out_nb_samples;
        wantSpec.callback = fill_audio;
        wantSpec.userdata = codecContext;

        if (SDL_OpenAudio(&wantSpec, NULL) < 0) {
            cout << "[error] open audio error" << endl;
            return;
        }

        // 初始化重采样器
        auConvertContext = swr_alloc_set_opts(NULL,
                                              out_chn_layout, out_sample_fmt, out_sample_rate,
                                              in_chn_layout, codecContext->sample_fmt, codecContext->sample_rate,
                                              0, NULL);
        // 初始化 SwResample 的 Context
        swr_init(auConvertContext);

        // 开始播放 调用这个方法硬件才会开始播放
        SDL_PauseAudio(0);
        // 循环读取 packet 并且解码
        int sendCode = 0;
        while (av_read_frame(formatContext, packet) >= 0) {
            if (packet->stream_index != audioIndex) continue;
            // 接受解码后的音频数据
            while (avcodec_receive_frame(codecContext, frame) == 0) {
                swr_convert(auConvertContext, &out_buffer, MAX_AUDIO_FRAME_SIZE,
                            (const uint8_t **) frame->data, frame->nb_samples);
                // 如果没有播放完就等待 1ms
                while (audioLen > 0) {
                    SDL_Delay(1);
                }
                // 同步数据
                audioChunk = out_buffer;
                audioPos = audioChunk;
                audioLen = out_buffer_size;
            }
            // 发送解码前的包数据
            sendCode = avcodec_send_packet(codecContext, packet);
            // 根据发送的返回值判断状态
            if (sendCode == 0) {
                cout << "[Debug]" << "SUCCESS" << endl;
            } else if (sendCode == AVERROR_EOF) {
                cout << "[Debug]" << "EOF" << endl;
            } else if (sendCode == AVERROR(EAGAIN)) {
                cout << "[Debug]" << "EAGAIN" << endl;
            } else {
                cout << "[Debug]" << av_err2str(AVERROR(sendCode)) << endl;
            }
            av_packet_unref(packet);
        }
    }

    void prepareFFmpeg(const char *url) {
        int retCode;
        // 初始化 FormatContext
        formatContext = avformat_alloc_context();
        if (!formatContext) {
            cout << "[error]alloc format context error!" << endl;
            return;
        }
        retCode = avformat_open_input(&formatContext, url, nullptr, nullptr);
        if (retCode != 0) {
            cout << "[error]open input error!" << endl;
            return;
        }
        retCode = avformat_find_stream_info(formatContext, NULL);
        if (retCode != 0) {
            cout << "[error] find stream error!" << endl;
            return;
        }

        codecContext = avcodec_alloc_context3(NULL);
        if (!codecContext) {
            cout << "[error] alloc codec context error!" << endl;
            return;
        }
        audioIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO,
                                         -1, -1, NULL, 0);
        retCode = avcodec_parameters_to_context(codecContext,
                                                formatContext->streams[audioIndex]->codecpar);
        if (retCode != 0) {
            cout << "[error] parameters to context error!" << endl;
            return;
        }

        codec = avcodec_find_decoder(codecContext->codec_id);
        if (codec == nullptr) {
            cout << "[error] find decoder error!" << endl;
            return;
        }
        retCode = avcodec_open2(codecContext, codec, nullptr);
        if (retCode != 0) {
            cout << "[error] open decoder error!" << endl;
            return;
        }

        packet = av_packet_alloc();
        frame = av_frame_alloc();
    }

    void free() {
        if (formatContext != nullptr) avformat_close_input(&formatContext);
        if (codecContext != nullptr) avcodec_free_context(&codecContext);
        if (packet != nullptr) av_packet_free(&packet);
        if (frame != nullptr) av_frame_free(&frame);
        if (auConvertContext != nullptr) swr_free(&auConvertContext);
        SDL_CloseAudio();
        SDL_Quit();
    }

}