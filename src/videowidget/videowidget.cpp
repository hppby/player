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
#include <QReadLocker>



VideoWidget::VideoWidget(QWidget *parent, QLabel *playerLabel)
        : QWidget(parent) {

    m_playView = playerLabel;
    // 初始化FFmpeg库
    avformat_network_init();
}

bool VideoWidget::openFile(QString fileName) {
    this->isRunning = this->initDecode(fileName);

    if (!this->isRunning) {
        this->closeFile();
        return false;
    }

    connect(this, &VideoWidget::videoImageChanged, this, [=](QPixmap pixmap){
        if (!pixmap.isNull()) {

            QSize size = this->m_playView->size();

//            if (size.width() / size.height() > pixmap.width() / pixmap.height()) {
//                pixmap.scaled(size.width(), size.height(), Qt::KeepAspectRatio);
//            } else {
//                pixmap.scaled(size.width(), size.height(), Qt::KeepAspectRatio);
//            };

            qDebug() << "===size===" << this->m_playView->size();

            this->m_playView->setPixmap(pixmap);
            pixmap.scaled(size.width() / 2, size.height()/2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
//            this->m_playView->setScaledContents(true);
        }
    });

    std::thread decodeThread(&VideoWidget::decodeLoop, this);
    decodeThread.detach();
    isChangedWindowSize = false;
    return true;
}

// 定义音频参数
const int SAMPLE_RATE = 44100; // 采样率，常见值如 44100, 48000 等
const Uint16 FORMAT = AUDIO_S16SYS; // 音频格式，AUDIO_S16SYS 代表系统默认的16位格式
const int CHANNELS = 2; // 声道数，1为单声道，2为立体声
const int BUFFER_SIZE = 1024; // 缓冲区大小，单位为样本数


// 清理并释放音频FIFO资源
void freeAudioFifo(AVAudioFifo **fifo) {
    if (*fifo) {
        av_audio_fifo_free(*fifo);
        *fifo = nullptr;
    }
}

bool VideoWidget::initSDL() {
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) { // 根据需要可以只初始化音频 SDL_INIT_AUDIO
        qDebug() << "SDL could not initialize! SDL_Error: " << SDL_GetError() ;
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
        // 处理Mix_OpenAudio错误
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return false;
    }
    Mix_VolumeMusic(MIX_MAX_VOLUME);
    audioFifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, CHANNELS, 1000);
    if (!audioFifo) {
        qDebug() << "Could not allocate audio FIFO." ;
        return false;
    }

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

void VideoWidget::closeFile() {

    qDebug() << "===closeFile===";

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
    if (video_sws_ctx) {
        sws_freeContext(video_sws_ctx);
        video_sws_ctx = nullptr;
    }

    // 停止播放并清理
    SDL_PauseAudioDevice(audioDeviceId, 1);
    SDL_CloseAudioDevice(audioDeviceId);
    SDL_Quit();
    // 清理
    freeAudioFifo(&audioFifo);
}

// 初始化SwrContext用于音频格式转换
SwrContext* VideoWidget::initSwrContext(AVCodecContext *audio_dec_ctx) {
    SwrContext *swrCtx = swr_alloc();
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
        qDebug() << "Failed to initialize the resampler context" ;
        swr_free(&swrCtx);
        return nullptr;
    }

    return swrCtx;
}

bool VideoWidget::initDecode(QString fileName) {
    this->closeFile(); // 先释放旧资源
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
    video_sws_ctx = sws_getContext(video_dec_ctx->width, video_dec_ctx->height,
                                   video_dec_ctx->pix_fmt,
                                   video_dec_ctx->width, video_dec_ctx->height,
                                   AV_PIX_FMT_RGB32,
                                   SWS_BILINEAR, nullptr, nullptr, nullptr);

    audio_swr_ctx = this->initSwrContext(audio_dec_ctx);

    if (!audio_swr_ctx) {
        qDebug() << "Could not allocate resampler context";
        return false;
    }

    if (!video_sws_ctx) {
        qDebug() << "初始化转换上下文--失败";
        return false;
    }

    if (!this->initSDL()) {
        qDebug() << "初始化SDL--失败";
        return false;
    }

    return true;
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



QImage VideoWidget::convertToQImage(AVFrame *src_frame, SwsContext *sws_ctx) {

    // 检查sws_ctx有效性，避免空指针调用
    if (!sws_ctx) {
        qDebug() << "SwsContext is null!";
        return QImage(); // 或者根据需要抛出异常
    }
  if (!src_frame) {
        qDebug() << "src_frame is null!";
        return QImage(); // 或者根据需要抛出异常
    }

    // 确保目标图像的大小与源帧匹配
    QImage dest(src_frame->width, src_frame->height, QImage::Format_RGB32);

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


// 处理解码后的AVFrame
void VideoWidget::processAudioFrame(AVFrame* frame) {

    // 注意：下面的代码应当使用frame而非新分配的input_frame
    const int outBufferSize = av_samples_get_buffer_size(nullptr, 2, frame->nb_samples, AV_SAMPLE_FMT_S16, 1); // 假设输出为双声道
    if (outBufferSize <= 0) {
        qDebug() << "Error calculating output buffer size." ;
        return;
    }

    // 分配输出缓冲区
    uint8_t* output_data = (uint8_t*)av_malloc(outBufferSize); // 动态分配单个缓冲区，而非指针数组

    // 转换音频数据
    int ret = swr_convert(audio_swr_ctx,
                          &output_data, frame->nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples); // 使用frame作为输入

    if (ret < 0) {
        qDebug() << "Error converting audio frame." ;
        return;
    }

    // 将转换后的音频数据送入SDL音频设备的队列
    int queued = SDL_QueueAudio(audioDeviceId, output_data, outBufferSize);
    if (queued < 0) {
        qDebug() << "SDL_QueueAudio failed: " << SDL_GetError() ;
    }

}

// 解码音频帧
void VideoWidget::decodeAudioFrame(AVPacket &pkt, AVFrame *audioFrame) {
    int sendResult = avcodec_send_packet(audio_dec_ctx, &pkt);
    if (sendResult < 0) {
        qDebug() << "Error sending packet to decoder: " << av_err2str(sendResult) ;
        return;
    }

    while (sendResult >= 0 && this->isRunning) {
        sendResult = avcodec_receive_frame(audio_dec_ctx, audioFrame);
        if (sendResult == AVERROR(EAGAIN) || sendResult == AVERROR_EOF) {
            break;
        } else if (sendResult < 0) {
            qDebug() << "Error during decoding: " << av_err2str(sendResult) ;
            return;
        }

        // 假设processAudioFrame处理音频帧
        this->processAudioFrame(audioFrame);
        av_frame_unref(audioFrame);
    }
}

void VideoWidget::decodeVideoFrame(AVPacket &pkt, AVFrame *decodedFrame) {

    if (!this->isRunning) {
        return;
    }

    qDebug() << "===decodeVideoFrame===" << this->isRunning;

    if (this->isRunning && avcodec_send_packet(video_dec_ctx, &pkt) >= 0) {
        qDebug() << "===1===";
        while (avcodec_receive_frame(video_dec_ctx, decodedFrame) == 0) {
            qDebug() << "===2===";
            if (!this->isRunning) {
                break;
            }
            qDebug() << "===3===";
            QImage newFrame = this->convertToQImage(decodedFrame, video_sws_ctx);
            QPixmap pixmap = QPixmap::fromImage(newFrame);
            if (!pixmap.isNull()) {
//                pixmap.scaled(this->videoSize, Qt::KeepAspectRatio);
//                this->m_playView->setPixmap(pixmap);
emit this->videoImageChanged(pixmap);
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
        };

        av_frame_unref(videoFrame);
        av_frame_unref(audioFrame);


        if (pkt.stream_index == video_stream_index) {
            this->decodeVideoFrame(pkt, videoFrame);

            // 计算当前时间
            AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base; // 获取视频流的时间基
            int64_t pts_time = pkt.pts != AV_NOPTS_VALUE ? pkt.pts : pkt.dts; // 使用pts或dts作为参考，优先pts
            double currentTime = av_q2d(time_base) * pts_time; // 将pts/dts转换为秒

            m_currentTime = currentTime;

        } else if (pkt.stream_index == audio_stream_index) {
            this->decodeAudioFrame(pkt, audioFrame);
        }


        av_packet_unref(&pkt);
    }

    av_frame_free(&videoFrame);
    av_frame_free(&audioFrame);

    if (isEnd) {
        this->closeFile();
    }
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

    // 释放资源
    this->closeFile();
}

void VideoWidget::onSoundOff() {
}


double VideoWidget::getCurrentTime() const {
//    QReadLocker locker(&rwLock); // 锁定读锁
    return this->m_currentTime;
}

double VideoWidget::getDuration() const {
//    QReadLocker locker(&rwLock);
    return this->m_duration;
}

void VideoWidget::changePlaybackProgress(int64_t target_time_seconds) {
    this->isRunning = false;
    // 目标播放时间，例如跳转到10秒处
    int64_t target_time_in_ts = av_rescale_q(target_time_seconds, AVRational{1, AV_TIME_BASE}, fmt_ctx->streams[video_stream_index]->time_base);

    // 寻找最近的关键帧
    int seek_result = av_seek_frame(fmt_ctx, video_stream_index, target_time_in_ts, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
    if (seek_result < 0) {
        fprintf(stderr, "Error seeking frame: %s\n", av_err2str(seek_result));
        return;
    }

    // 重置解码器
    avcodec_flush_buffers(video_dec_ctx);
    this->isRunning = true;

    auto *thread = new QThread(this);

    std::thread decodeThread(&VideoWidget::decodeLoop, this);
    decodeThread.detach();
}

bool VideoWidget::stop() {
    this->isRunning = false;
    return false;
}




