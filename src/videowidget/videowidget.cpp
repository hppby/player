//
// Created by LOOG LS on 2024/6/11.
//

#include "videowidget.h"
#include <QPainter>
#include <QMutex>
#include <QWaitCondition>
#include <thread>
#include <QThread>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>





// 定义一些全局变量和结构体
AVFormatContext *fmt_ctx = nullptr;
AVCodecContext *video_dec_ctx = nullptr;
AVCodecContext *audio_dec_ctx = nullptr;
AVStream *video_stream = nullptr;
AVStream *audio_stream = nullptr;

int video_stream_index = -1; // 初始化为-1，表示尚未找到视频流
int audio_stream_index = -1;

AVFrame *frame = nullptr;
struct SwsContext *sws_ctx = nullptr;
QMutex mutex;
QWaitCondition waitCond;


QImage m_latestFrame; // 用于存储最新解码的图像
QMutex m_frameMutex; // 用于保护m_latestFrame的访问
QWaitCondition m_frameAvailable; // 用于通知新帧可用

// 初始化了SDL并创建了音频设备
SDL_AudioDeviceID audioDeviceId;

// 音频FIFO
AVAudioFifo *audioFifo;

VideoWidget::VideoWidget(QWidget *parent, QLabel *playerLabel)
        : QWidget(parent) {

    m_playView = playerLabel;

    // 初始化FFmpeg库
    avformat_network_init();
}


// 定义音频参数
const int SAMPLE_RATE = 44100; // 采样率，常见值如 44100, 48000 等
const Uint16 FORMAT = AUDIO_S16SYS; // 音频格式，AUDIO_S16SYS 代表系统默认的16位格式
const int CHANNELS = 2; // 声道数，1为单声道，2为立体声
const int BUFFER_SIZE = 1024; // 缓冲区大小，单位为样本数
// 设置音频参数
SDL_AudioSpec desiredSpec;

// 初始化音频FIFO
AVAudioFifo *initAudioFifo(enum AVSampleFormat sample_fmt, int channels, int nb_samples) {
    AVAudioFifo *fifo = av_audio_fifo_alloc(sample_fmt, channels, nb_samples);
    if (!fifo) {
        std::cerr << "Could not allocate audio FIFO." << std::endl;
        return nullptr;
    }
    return fifo;
}

// 清理并释放音频FIFO资源
void freeAudioFifo(AVAudioFifo **fifo) {
    if (*fifo) {
        av_audio_fifo_free(*fifo);
        *fifo = nullptr;
    }
}

bool initSDL() {
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) { // 根据需要可以只初始化音频 SDL_INIT_AUDIO
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
        // 处理Mix_OpenAudio错误
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return false;
    }
    Mix_VolumeMusic(MIX_MAX_VOLUME);
    audioFifo = initAudioFifo(AV_SAMPLE_FMT_S16, CHANNELS, 1000); // 示例参数，根据实际情况调整


    desiredSpec.freq = SAMPLE_RATE;
    desiredSpec.format = FORMAT;
    desiredSpec.channels = CHANNELS;
    desiredSpec.samples = BUFFER_SIZE; // 单次回调的样本数
    desiredSpec.callback = nullptr; // 如果使用队列方式，则不需要回调
    desiredSpec.userdata = nullptr;

    // 打开音频设备
    audioDeviceId = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, nullptr, 0);
    if (audioDeviceId == 0) {
        SDL_Quit();
        return false;
    }

    // 开始音频播放
    SDL_PauseAudioDevice(audioDeviceId, 0);

    return true;
}


void closeFile() {

    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
        fmt_ctx = nullptr;
    }
    if (video_dec_ctx) {
        avcodec_free_context(&video_dec_ctx);
        video_dec_ctx = nullptr;
    }
    if (audio_dec_ctx) {
        avcodec_free_context(&audio_dec_ctx);
        audio_dec_ctx = nullptr;
    }
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
        sws_ctx = nullptr;
    }
    if (frame) {
        av_frame_free(&frame);
        frame = nullptr;
    }

    // 停止播放并清理
    SDL_PauseAudioDevice(audioDeviceId, 1);
    SDL_CloseAudioDevice(audioDeviceId);
    SDL_Quit();
    // 清理
    freeAudioFifo(&audioFifo);
}

QImage VideoWidget::convertToQImage(AVFrame *src_frame, SwsContext *sws_ctx) {
    // 确保目标图像的大小与源帧匹配
    QImage dest(src_frame->width, src_frame->height, QImage::Format_RGB32);

    // 检查sws_ctx有效性，避免空指针调用
    if (!sws_ctx) {
        qDebug() << "SwsContext is null!";
        return QImage(); // 或者根据需要抛出异常
    }

    // 设置转换参数
    uint8_t *dst_data[4] = {dest.bits(), nullptr, nullptr, nullptr}; // RGB32格式只需第一个指针
    int dst_linesize[4]; // 其余为0，因为RGB32是平面格式
    dst_linesize[0] = static_cast<int>(dest.bytesPerLine()); // 显式类型转换
    dst_linesize[1] = 0;
    dst_linesize[2] = 0;
    dst_linesize[3] = 0;

    // 执行转换并检查是否成功
    int result = sws_scale(sws_ctx, src_frame->data, src_frame->linesize, 0, src_frame->height,
                           dst_data, dst_linesize);
    if (result <= 0) {
        qDebug() << "sws_scale failed with error code:" << result;
        return QImage(); // 或者根据需要处理错误
    }

    // 返回转换后的图像
    return dest;
}


static SwrContext* swrCtx = nullptr;
// 初始化SwrContext用于音频格式转换
SwrContext* VideoWidget::initSwrContext(AVCodecContext *&audio_dec_ctx) {
   swrCtx = swr_alloc();
    if (!swrCtx) {
        qDebug() << "Could not allocate resampler context";
        return nullptr;
    }

    av_opt_set_int(swrCtx, "in_channel_layout", av_get_default_channel_layout(audio_dec_ctx->channels), 0);
    av_opt_set_int(swrCtx, "out_channel_layout", av_get_default_channel_layout(2), 0);
    av_opt_set_int(swrCtx, "in_sample_rate", audio_dec_ctx->sample_rate, 0);
    av_opt_set_int(swrCtx, "out_sample_rate", audio_dec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swrCtx, "in_sample_fmt", audio_dec_ctx->sample_fmt, 0);
    av_opt_set_sample_fmt(swrCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

    if (swr_init(swrCtx) < 0) {
        std::cerr << "Failed to initialize the resampler context" << std::endl;
        swr_free(&swrCtx);
        return nullptr;
    }

    return swrCtx;
}





// 处理解码后的AVFrame
void VideoWidget::processAudioFrame(AVFrame* frame, AVCodecContext *&audio_dec_ctx) {
   // 使用静态变量保持SwrContext实例，避免每次调用都重新创建

    SwrContext *swrCtx = this->initSwrContext(audio_dec_ctx);

    if (!swrCtx) {
        qDebug() << "Could not allocate resampler context";
        return;
    }

    // 直接使用传入的frame，无需再次分配input_frame
    // 注意：下面的代码应当使用frame而非新分配的input_frame
    const int outBufferSize = av_samples_get_buffer_size(nullptr, 2, frame->nb_samples, AV_SAMPLE_FMT_S16, 1); // 假设输出为双声道
    if (outBufferSize <= 0) {
        std::cerr << "Error calculating output buffer size." << std::endl;
        return;
    }

    // 分配输出缓冲区
    uint8_t* output_data = (uint8_t*)av_malloc(outBufferSize); // 动态分配单个缓冲区，而非指针数组

    // 转换音频数据
    int ret = swr_convert(swrCtx,
                          &output_data, frame->nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples); // 使用frame作为输入

    if (ret < 0) {
        std::cerr << "Error converting audio frame." << std::endl;
        return;
    }

    // 将转换后的音频数据送入SDL音频设备的队列
    int queued = SDL_QueueAudio(audioDeviceId, output_data, outBufferSize);
    if (queued < 0) {
        std::cerr << "SDL_QueueAudio failed: " << SDL_GetError() << std::endl;
    }

}


// 解码音频帧
void VideoWidget::decodeAudioFrame(AVPacket &pkt, AVFrame *&audioFrame, AVCodecContext *&audio_dec_ctx) {
    int sendResult = avcodec_send_packet(audio_dec_ctx, &pkt);
    if (sendResult < 0) {
        std::cerr << "Error sending packet to decoder: " << av_err2str(sendResult) << std::endl;
        return;
    }

    while (sendResult >= 0 && this->isRunning) {
        sendResult = avcodec_receive_frame(audio_dec_ctx, audioFrame);
        if (sendResult == AVERROR(EAGAIN) || sendResult == AVERROR_EOF) {
            break;
        } else if (sendResult < 0) {
            std::cerr << "Error during decoding: " << av_err2str(sendResult) << std::endl;
            return;
        }

        // 假设processAudioFrame处理音频帧
        this->processAudioFrame(audioFrame, audio_dec_ctx);
        av_frame_unref(audioFrame);
    }
}

void VideoWidget::decodeVideoFrame(AVPacket &pkt, AVFrame *&decodedFrame, AVCodecContext *&dec_ctx, QMutex &mutex, SwsContext *&sws_ctx,
                 QLabel *&m_playView) {
    if (avcodec_send_packet(dec_ctx, &pkt) >= 0) {
        while (avcodec_receive_frame(dec_ctx, decodedFrame) == 0) {
            QImage newFrame;
            {
                QMutexLocker locker(&mutex);
                newFrame = this->convertToQImage(decodedFrame, sws_ctx);
                QPixmap pixmap = QPixmap::fromImage(newFrame);
                pixmap.scaled(m_playView->size(), Qt::KeepAspectRatio);
                m_playView->setScaledContents(true);
                m_playView->setPixmap(pixmap);
            }
        }
    }
    av_frame_unref(decodedFrame);
}


void VideoWidget::decodeLoop() {
    AVFrame *videoFrame = av_frame_alloc();
    AVFrame *audioFrame = av_frame_alloc();

    bool isEnd = false;

    while (!isEnd && this->isRunning) {
        AVPacket pkt;
        if (av_read_frame(fmt_ctx, &pkt) < 0) {
            isEnd = true;
            break;
        }; // 文件读取完毕或错误

        av_frame_unref(videoFrame);
        av_frame_unref(audioFrame);

        if (pkt.stream_index == video_stream_index) {
            this->decodeVideoFrame(pkt, videoFrame, video_dec_ctx, mutex, sws_ctx, m_playView);

            // 计算当前时间
            AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base; // 获取视频流的时间基
            int64_t pts_time = pkt.pts != AV_NOPTS_VALUE ? pkt.pts : pkt.dts; // 使用pts或dts作为参考，优先pts
            double currentTime = av_q2d(time_base) * pts_time; // 将pts/dts转换为秒

            this->m_currentTime = currentTime;
        } else if (pkt.stream_index == audio_stream_index) {
            this->decodeAudioFrame(pkt, audioFrame, audio_dec_ctx);
        }

        av_packet_unref(&pkt);
    }

    av_frame_free(&videoFrame);
    av_frame_free(&audioFrame);

    if (isEnd) {
        closeFile();
    }
}




bool VideoWidget::openFile(QString fileName) {
    this->isRunning = this->initDecode(fileName);

    std::thread decodeThread(&VideoWidget::decodeLoop, this);
    decodeThread.detach();
    if (!this->isRunning) {
        closeFile();
        return false;
    }
    return true;
}

bool VideoWidget::initDecode(QString fileName) {
    closeFile(); // 先释放旧资源
    QByteArray ba = fileName.toLocal8Bit();
    const char *c_filename = ba.data();

    // 打开视频文件
    if (!this->openInputFile(c_filename)) return false;

    // 获取流信息
    if (!this->fetchStreamInfo()) return false;

    // 初始化视频解码器
    if (!this->initDecoder(video_stream_index, &video_dec_ctx)) return false;

    // 初始化音频解码器
    if (audio_stream_index >= 0 && !this->initDecoder(audio_stream_index, &audio_dec_ctx)) return false;

    // 初始化转换上下文
    sws_ctx = sws_getContext(video_dec_ctx->width, video_dec_ctx->height,
                             video_dec_ctx->pix_fmt,
                             video_dec_ctx->width, video_dec_ctx->height,
                             AV_PIX_FMT_RGB32,
                             SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws_ctx) {
        qDebug() << "初始化转换上下文--失败";
        return false;
    }

    if (!initSDL()) {
        qDebug() << "初始化SDL--失败";
        return false;
    }

    return true; // 成功
}

// 辅助函数
bool VideoWidget::openInputFile(const char *filename) {
    if (avformat_open_input(&fmt_ctx, filename, nullptr, nullptr) != 0) {
        qDebug() << "打开视频文件--失败";
        return false;
    }

    return true;
}

bool VideoWidget::fetchStreamInfo() {
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        qDebug() << "获取流信息--失败";
        return false;
    }

    this->m_duration = (double)fmt_ctx->duration / AV_TIME_BASE;

    video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (video_stream_index < 0) {
        qDebug() << "查找视频流--失败";
        return false;
    }
    video_stream = fmt_ctx->streams[video_stream_index];
    audio_stream = fmt_ctx->streams[audio_stream_index];
    qDebug() << "找到视频流";
    return true;
}

bool VideoWidget::initDecoder(int streamIndex, AVCodecContext **ctx) {
    const AVCodecParameters *codecpar = fmt_ctx->streams[streamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        qDebug() << QString("查找解码器--失败 (流类型: %1)").arg(streamIndex == video_stream_index ? "视频" : "音频");
        return false;
    }
    *ctx = avcodec_alloc_context3(codec);
    if (!(*ctx)) {
        qDebug() << QString("分配解码器上下文--失败 (流类型: %1)").arg(
                streamIndex == video_stream_index ? "视频" : "音频");
        return false;
    }
    if (avcodec_parameters_to_context(*ctx, codecpar) < 0) {
        qDebug() << QString("复制编解码参数到解码上下文--失败 (流类型: %1)").arg(
                streamIndex == video_stream_index ? "视频" : "音频");
        return false;
    }
    if (avcodec_open2(*ctx, codec, nullptr) < 0) {
        qDebug() << QString("打开解码器--失败 (流类型: %1)").arg(streamIndex == video_stream_index ? "视频" : "音频");
        return false;
    }
    return true;
}



bool VideoWidget::startOrPause() {
    if (this->isRunning) {
        this->isRunning = false;
        SDL_PauseAudioDevice(audioDeviceId, 1);
    } else {
        this->isRunning = true;
        SDL_PauseAudioDevice(audioDeviceId, 0);
        std::thread decodeThread(&VideoWidget::decodeLoop, this);
        decodeThread.detach();
    }

    return this->isRunning;
}

void VideoWidget::changeVolume(int volume) {
// 设置音量，范围为0（静音）到MIX_MAX_VOLUME（最大音量，通常是128）
    Mix_VolumeMusic(MIX_MAX_VOLUME * volume / 100);
    Mix_Volume(2, MIX_MAX_VOLUME * volume / 100);
    qDebug() << "===changeVolume===" << volume;
}

VideoWidget::~VideoWidget() {
    // 发送信号让解码线程退出循环
    {
        QMutexLocker locker(&mutex);
        // 设置退出标志，这里假设有一个bool成员m_exitFlag
        m_exitFlag = true;
        waitCond.wakeAll();
    }

    // 等待解码线程结束（在某些情况下可能需要）
    // 实际应用中可能需要更复杂的线程管理

    // 释放资源
    closeFile();
}


