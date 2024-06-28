//
// Created by LOOG LS on 2024/6/26.
//


#include "DecodeAudio.h"
#include <SDL2/SDL_mixer.h>
#include <QDebug>
#include <QThreadPool>
#include "AudioDecoderThread.h"

// 定义音频参数
const int SAMPLE_RATE = 44100; // 采样率，常见值如 44100, 48000 等
const Uint16 FORMAT = AUDIO_S16SYS; // 音频格式，AUDIO_S16SYS 代表系统默认的16位格式
const int CHANNELS = 2; // 声道数，1为单声道，2为立体声
const int BUFFER_SIZE = 1024; // 缓冲区大小，单位为样本数

DecodeAudio::DecodeAudio(AVFormatContext *fmtCtx) {
    this->fmt_ctx = fmtCtx;
    this->packet_queue = new QQueue<AVPacket *>();
    this->is_playing = false;
}

bool DecodeAudio::init() {
    bool ret = this->_init();
    if (!ret) {
        qDebug() << "初始化--失败";
        this->closeFile();
    }

    ret = this->initSDL();
    if (!ret) {
        qDebug() << "初始化SDL--失败";
        this->closeFile();
    }
    return ret;
}

bool DecodeAudio::_init() {

    // 查找视频流
    audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index < 0) {
        qDebug() << "查找视频流--失败";
        return false;
    }
    audio_stream = fmt_ctx->streams[audio_stream_index];

    // 初始化音频解码器
    if (!this->initAudioDecoder()) return false;

    return true;
}


bool DecodeAudio::initAudioDecoder() {
    const AVCodecParameters *codecpar = audio_stream->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        qDebug() << QString("查找解码器--失败 (流类型: %1)").arg("音频");
        return false;
    }

    audio_dec_ctx = avcodec_alloc_context3(codec);
    if (!(audio_dec_ctx)) {
        qDebug() << QString("分配解码器上下文--失败 (流类型: %1)").arg("音频");
        return false;
    }
    if (avcodec_parameters_to_context(audio_dec_ctx, codecpar) < 0) {
        qDebug() << QString("复制编解码参数到解码上下文--失败 (流类型: %1)").arg("音频");
        return false;
    }
    if (avcodec_open2(audio_dec_ctx, codec, nullptr) < 0) {
        qDebug() << QString("打开解码器--失败 (流类型: %1)").arg("音频");
        return false;
    }

    audio_swr_ctx = swr_alloc();
    if (!audio_swr_ctx) {
        qDebug() << "Could not allocate resampler context";
        return false;
    }

    av_opt_set_int(audio_swr_ctx, "in_channel_layout",
                   av_get_default_channel_layout(audio_dec_ctx->ch_layout.nb_channels), 0);
    av_opt_set_int(audio_swr_ctx, "out_channel_layout",
                   av_get_default_channel_layout(audio_dec_ctx->ch_layout.nb_channels), 0);
    av_opt_set_int(audio_swr_ctx, "in_sample_rate", audio_dec_ctx->sample_rate, 0);
    av_opt_set_int(audio_swr_ctx, "out_sample_rate", audio_dec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(audio_swr_ctx, "in_sample_fmt", audio_dec_ctx->sample_fmt, 0);
    av_opt_set_sample_fmt(audio_swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    if (swr_init(audio_swr_ctx) < 0) {
        qDebug() << "Failed to initialize the resampler context";
        swr_free(&audio_swr_ctx);
        return false;
    }

    return true;
}


// 处理解码后的AVFrame
void DecodeAudio::processAudioFrame(AVFrame *frame) {

    // 注意：下面的代码应当使用frame而非新分配的input_frame
    const int outBufferSize = av_samples_get_buffer_size(nullptr, 2, frame->nb_samples, AV_SAMPLE_FMT_S16,
                                                         1); // 假设输出为双声道
    if (outBufferSize <= 0) {
        qDebug() << "Error calculating output buffer size.";
        return;
    }

    // 分配输出缓冲区
    uint8_t *output_data = (uint8_t *) av_malloc(outBufferSize);

    // 转换音频数据
    int ret = swr_convert(audio_swr_ctx,
                          &output_data, frame->nb_samples,
                          (const uint8_t **) frame->data, frame->nb_samples); // 使用frame作为输入

    if (ret < 0) {
        qDebug() << "Error converting audio frame.";
        return;
    }

    // 将转换后的音频数据送入SDL音频设备的队列
    int queued = SDL_QueueAudio(audio_device_id, output_data, outBufferSize);
    if (queued < 0) {
        qDebug() << "SDL_QueueAudio failed: " << SDL_GetError();
        return;
    }
}

// 解码音频帧
void DecodeAudio::decodeAudioFrame(AVPacket *pkt) {


    AVFrame *audioFrame = av_frame_alloc();
    int sendResult = avcodec_send_packet(audio_dec_ctx, pkt);
    if (sendResult < 0) {
        qDebug() << "Error sending packet to decoder: " << av_err2str(sendResult);
        return;
    }

    while (sendResult >= 0 && this->is_playing) {
        sendResult = avcodec_receive_frame(audio_dec_ctx, audioFrame);
        if (sendResult == AVERROR(EAGAIN) || sendResult == AVERROR_EOF) {
            qDebug() << "Error during decoding == 1: " << av_err2str(sendResult);
            break;
        } else if (sendResult < 0) {
            qDebug() << "Error during decoding == 2: " << av_err2str(sendResult);
            return;
        }

        this->processAudioFrame(audioFrame);
    }
    av_frame_free(&audioFrame);
}


void DecodeAudio::decodeLoop() {

    qDebug() << "===DecodeAudio::decodeLoop===";

    while (this->is_playing) {

        if (this->packet_queue->empty()) {
            av_usleep(1000);
            continue;

        }

        AVPacket *pkt = this->packet_queue->dequeue();

        if (pkt) {
            this->decodeAudioFrame(pkt);
        }

        av_packet_free(&pkt);
    }

}

bool DecodeAudio::initSDL() {
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) { // 根据需要可以只初始化音频 SDL_INIT_AUDIO
        qDebug() << "SDL could not initialize! SDL_Error: " << SDL_GetError();
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
        // 处理Mix_OpenAudio错误
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return false;
    }
    Mix_VolumeMusic(MIX_MAX_VOLUME);

    const AVCodecParameters *codecpar = audio_stream->codecpar;


    desired_spec.freq = codecpar->sample_rate;
    desired_spec.format = FORMAT;
    desired_spec.channels = CHANNELS;
    desired_spec.samples = BUFFER_SIZE; // 单次回调的样本数
    desired_spec.callback = nullptr; // 如果使用队列方式，则不需要回调
    desired_spec.userdata = nullptr;

    // 打开音频设备
    audio_device_id = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, nullptr, 0);
    if (audio_device_id == 0) {
        SDL_Quit();
        return false;
    }

    // 开始音频播放
    SDL_PauseAudioDevice(audio_device_id, 0);

    return true;
}


void DecodeAudio::closeFile() {

    qDebug() << "===closeFile===";

    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
        fmt_ctx = nullptr;
    }

    if (audio_dec_ctx) {
        avcodec_free_context(&audio_dec_ctx);
        audio_dec_ctx = nullptr;
    }

    // 停止播放并清理
    SDL_PauseAudioDevice(audio_device_id, 1);
    SDL_CloseAudioDevice(audio_device_id);
    SDL_Quit();

}


bool DecodeAudio::startOrPause(bool pause) {
    this->is_playing = pause;
    if (this->is_playing) {
        SDL_PauseAudioDevice(audio_device_id, 0);
        audioDecoderThread = new AudioDecoderThread(this);
        QThreadPool::globalInstance()->start(audioDecoderThread);
    } else {
        SDL_PauseAudioDevice(audio_device_id, 1);
        audioDecoderThread = nullptr;
    }

    return this->is_playing;
}


DecodeAudio::~DecodeAudio() {
    this->closeFile();
}

void DecodeAudio::addPacket(AVPacket *pkt) {
    AVPacket *pkt1 = av_packet_alloc();
    if (!pkt1) {
        av_packet_unref(pkt1);
    }
    av_packet_move_ref(pkt1, pkt);

    my_mutex.lock();
    this->packet_queue->enqueue(pkt1);
    my_mutex.unlock();
}

void DecodeAudio::resetPacketQueue() {
    my_mutex.lock();
    this->packet_queue->clear();

    my_mutex.unlock();
}
