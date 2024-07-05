//
// Created by LOOG LS on 2024/6/26.
//


#include "DecodeAudio.h"
#include <SDL2/SDL_mixer.h>
#include <QDebug>
#include <QThreadPool>
#include "AudioDecoderThread.h"
#include "DecodeState.h"
#include "AudioPlayThread.h"

// 定义音频参数
const int SAMPLE_RATE = 44100; // 采样率，常见值如 44100, 48000 等
const Uint16 FORMAT = AUDIO_S16SYS; // 音频格式，AUDIO_S16SYS 代表系统默认的16位格式
const int CHANNELS = 2; // 声道数，1为单声道，2为立体声
const int BUFFER_SIZE = 1024; // 缓冲区大小，单位为样本数

DecodeAudio::DecodeAudio(AVFormatContext *fmtCtx) {
//    this->packet_queue = new QQueue<AVPacket *>();
}

bool DecodeAudio::init() {
    bool ret = this->_init();
    if (!ret) {
        qDebug() << "初始化--失败";
        this->closeFile();
    }

    return ret;
}

bool DecodeAudio::_init() {

    // 查找视频流
    DecodeState::getInstance()->audio_stream_index = av_find_best_stream(
            DecodeState::getInstance()->fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (DecodeState::getInstance()->audio_stream_index < 0) {
        qDebug() << "查找视频流--失败";
        return false;
    }
    DecodeState::getInstance()->audio_stream = DecodeState::getInstance()->fmt_ctx->streams[DecodeState::getInstance()->audio_stream_index];

    // 初始化音频解码器
    if (!this->initAudioDecoder()) return false;

    return true;
}


bool DecodeAudio::initAudioDecoder() {
    const AVCodecParameters *codecpar = DecodeState::getInstance()->audio_stream->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        qDebug() << QString("查找解码器--失败 (流类型: %1)").arg("音频");
        return false;
    }

    DecodeState::getInstance()->audio_dec_ctx = avcodec_alloc_context3(codec);
    if (!(DecodeState::getInstance()->audio_dec_ctx)) {
        qDebug() << QString("分配解码器上下文--失败 (流类型: %1)").arg("音频");
        return false;
    }
    if (avcodec_parameters_to_context(DecodeState::getInstance()->audio_dec_ctx, codecpar) < 0) {
        qDebug() << QString("复制编解码参数到解码上下文--失败 (流类型: %1)").arg("音频");
        return false;
    }
    if (avcodec_open2(DecodeState::getInstance()->audio_dec_ctx, codec, nullptr) < 0) {
        qDebug() << QString("打开解码器--失败 (流类型: %1)").arg("音频");
        return false;
    }

    DecodeState::getInstance()->audio_swr_ctx = swr_alloc();
    if (!DecodeState::getInstance()->audio_swr_ctx) {
        qDebug() << "Could not allocate resampler context";
        return false;
    }

    av_opt_set_int(DecodeState::getInstance()->audio_swr_ctx, "in_channel_layout",
                   av_get_default_channel_layout(DecodeState::getInstance()->audio_dec_ctx->ch_layout.nb_channels), 0);
    av_opt_set_int(DecodeState::getInstance()->audio_swr_ctx, "out_channel_layout",
                   av_get_default_channel_layout(DecodeState::getInstance()->audio_dec_ctx->ch_layout.nb_channels), 0);
    av_opt_set_int(DecodeState::getInstance()->audio_swr_ctx, "in_sample_rate",
                   DecodeState::getInstance()->audio_dec_ctx->sample_rate, 0);
    av_opt_set_int(DecodeState::getInstance()->audio_swr_ctx, "out_sample_rate",
                   DecodeState::getInstance()->audio_dec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(DecodeState::getInstance()->audio_swr_ctx, "in_sample_fmt",
                          DecodeState::getInstance()->audio_dec_ctx->sample_fmt, 0);
    av_opt_set_sample_fmt(DecodeState::getInstance()->audio_swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    if (swr_init(DecodeState::getInstance()->audio_swr_ctx) < 0) {
        qDebug() << "Failed to initialize the resampler context";
        swr_free(&DecodeState::getInstance()->audio_swr_ctx);
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
    int ret = swr_convert(DecodeState::getInstance()->audio_swr_ctx,
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
    if (!DecodeState::getInstance()->is_playing) {
        av_packet_unref(pkt);
        av_packet_free(&pkt);
        return;
    };

    int sendResult = avcodec_send_packet(DecodeState::getInstance()->audio_dec_ctx, pkt);
    av_packet_free(&pkt);
    if (sendResult < 0) {
        qDebug() << "Error sending packet to decoder: " << av_err2str(sendResult);
        return;
    }

    while (true) {
        AVFrame *audioFrame = av_frame_alloc();
        int receiveResult = avcodec_receive_frame(DecodeState::getInstance()->audio_dec_ctx, audioFrame);

        if (receiveResult == AVERROR(EAGAIN) || receiveResult == AVERROR_EOF) {
            av_frame_free(&audioFrame);
            break;
        } else if (receiveResult < 0) {
            qDebug() << "Error during decoding == 2: " << av_err2str(receiveResult);
            av_frame_free(&audioFrame);
            break;
        } else {
            DecodeState::getInstance()->addAudioFrame(audioFrame);
        }
    }
    qDebug() << "===解码结束==2=";
}


void DecodeAudio::decodeLoop() {

    qDebug() << "===DecodeAudio::decodeLoop===开始==";

    int64_t startTime = av_gettime();
    DecodeState *state = DecodeState::getInstance();

    while (state->is_decoding) {

        AVPacket *pkt = state->getAudioPacket();

        if (pkt) {
            // 计算当前时间
            double audio_pts =
                    av_q2d(state->fmt_ctx->streams[state->audio_stream_index]->time_base) * pkt->pts;

            int64_t currentTime = audio_pts * 1000 * 1000 + startTime;

            int64_t diff = currentTime - av_gettime();

            if (diff < 0) {
                continue;
            }
            this->decodeAudioFrame(pkt);
        } else {
            av_usleep(1000);
        }

    }

    qDebug() << "===DecodeAudio::decodeLoop===结束==";
}

void DecodeAudio::playLoop() {

    double audioPts = 0.0;
    qDebug() << "===DecodeAudio::playLoop===开始==";

    int64_t startTime = av_gettime();

    DecodeState *state = DecodeState::getInstance();

    while (state->is_playing) {


        AVFrame *frame = state->getAudioFrame();
        if (frame) {
            // 计算当前时间
            audioPts = av_q2d(state->fmt_ctx->streams[state->audio_stream_index]->time_base) * frame->pts;

            int64_t currentTime = (audioPts - state->start_time) * 1000 * 1000 + startTime;
            duration = audioPts;
            int64_t diff = currentTime - av_gettime();

            if (diff < -10 * 1000) {
                qDebug() << "===diff==continue=" << diff;
                av_frame_free(&frame);
                continue;
            }

            this->processAudioFrame(frame);
            av_frame_free(&frame);
        } else {
            av_usleep(100);
        }
    }
    qDebug() << "===DecodeAudio::decodeLoop===结束==";
}


bool DecodeAudio::initSDL() {
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        // 根据需要可以只初始化音频 SDL_INIT_AUDIO
        qDebug() << "SDL could not initialize! SDL_Error: " << SDL_GetError();
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
        // 处理Mix_OpenAudio错误
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return false;
    }
    Mix_VolumeMusic(MIX_MAX_VOLUME);

    const AVCodecParameters *codecpar = DecodeState::getInstance()->audio_stream->codecpar;


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

    if (DecodeState::getInstance()->fmt_ctx) {
        avformat_close_input(&DecodeState::getInstance()->fmt_ctx);
        DecodeState::getInstance()->fmt_ctx = nullptr;
    }

    if (DecodeState::getInstance()->audio_dec_ctx) {
        avcodec_free_context(&DecodeState::getInstance()->audio_dec_ctx);
        DecodeState::getInstance()->audio_dec_ctx = nullptr;
    }

    // 停止播放并清理
    SDL_PauseAudioDevice(audio_device_id, 1);
    SDL_CloseAudioDevice(audio_device_id);
    SDL_Quit();

}


bool DecodeAudio::playAndPause(bool pause) {
    DecodeState::getInstance()->is_playing = pause;
    if (DecodeState::getInstance()->is_playing) {
        SDL_PauseAudioDevice(audio_device_id, 0);
        audioPlayThread = new AudioPlayThread(this);
        QThreadPool::globalInstance()->start(audioPlayThread);
    } else {
        SDL_PauseAudioDevice(audio_device_id, 1);
        audioPlayThread = nullptr;
    }

    return DecodeState::getInstance()->is_playing;
}


DecodeAudio::~DecodeAudio() {
    this->closeFile();
}


void DecodeAudio::stop() {
    SDL_PauseAudioDevice(audio_device_id, 1);
    audioDecoderThread = nullptr;
}

void DecodeAudio::start() {
    bool ret = this->initSDL();
    if (!ret) {
        qDebug() << "初始化SDL--失败";
        this->closeFile();
        return;
    }
    this->playAndPause(true);
}

void DecodeAudio::startDecode() {
    qDebug() << "===开始解码===";

    DecodeState::getInstance()->is_decoding = true;

    audioDecoderThread = new AudioDecoderThread(this);
    QThreadPool::globalInstance()->start(audioDecoderThread);
}

void DecodeAudio::stopDecode() {
    qDebug() << "===结束解码===";
    DecodeState::getInstance()->is_decoding = false;

}
