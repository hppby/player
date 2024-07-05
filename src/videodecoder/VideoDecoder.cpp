//
// Created by LOOG LS on 2024/6/11.
//

#include "VideoDecoder.h"
#include <QPainter>
#include <QMutex>
#include <QWaitCondition>
#include <thread>
#include <QThread>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <QReadLocker>

#include <unistd.h>
#include "ReadFrameThread.h"
#include "DecodeVideo.h"
#include "DecodeAudio.h"
#include "DecodeState.h"


VideoDecoder::VideoDecoder(QWidget *parent, QLabel *playerLabel)
        : QObject(parent) {

    m_playView = playerLabel;
    // 初始化FFmpeg库
    avformat_network_init();
    DecodeState::getInstance()->is_playing = false;
}

bool VideoDecoder::openFile(QString fileName) {

    this->initDecode(fileName);


    isChangedWindowSize = false;
    return true;
}


bool VideoDecoder::initDecode(QString fileName) {
    QByteArray ba = fileName.toLocal8Bit();
    const char *c_filename = ba.data();

    fmt_ctx = avformat_alloc_context();
    if (!fmt_ctx) {
        qDebug() << "===Could not allocate context===";
        return false;
    }

    DecodeState::getInstance()->fmt_ctx = fmt_ctx;

    // 打开视频文件
    if (avformat_open_input(&fmt_ctx, c_filename, nullptr, nullptr) != 0) {
        qDebug() << "打开视频文件--失败";
        return false;
    }

    // 获取流信息
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        qDebug() << "获取流信息--失败";
        return false;
    }

    decode_video = new DecodeVideo(this, fmt_ctx);
    if (!decode_video->init()) {
        qDebug() << "初始化视频解码器--失败";
        return false;
    }

    connect(decode_video, &DecodeVideo::videoImageChanged, this, &VideoDecoder::videoImageChanged);
    connect(decode_video, &DecodeVideo::currentTimeChanged, this, &VideoDecoder::currentTimeChanged);


    decode_audio = new DecodeAudio(fmt_ctx);
    if (!decode_audio->init()) {
        qDebug() << "初始化音频解码器--失败";
        return false;
    }

    this->startRead();

    // 开始
    connect(this, &VideoDecoder::start, decode_audio, &DecodeAudio::start);
    connect(this, &VideoDecoder::start, decode_video, &DecodeVideo::start);

    // 播放 / 暂停
    connect(this, &VideoDecoder::stop, decode_audio, &DecodeAudio::stop);
    connect(this, &VideoDecoder::stop, decode_video, &DecodeVideo::stop);

    // 停止
    connect(this, &VideoDecoder::playAndPause, decode_audio, &DecodeAudio::playAndPause);
    connect(this, &VideoDecoder::playAndPause, decode_video, &DecodeVideo::playAndPause);

    this->m_duration = (double) fmt_ctx->duration / AV_TIME_BASE;

    this->onStart();
    return true;
}

void VideoDecoder::readFrameLoop() {

    qDebug() << "===VideoDecoder::readFrameLoop== 开始 =";
    DecodeState *state = DecodeState::getInstance();
    DecodeState::getInstance()->clearQueue();
    avcodec_flush_buffers(DecodeState::getInstance()->video_dec_ctx);
    avcodec_flush_buffers(DecodeState::getInstance()->audio_dec_ctx);
    AVPacket *pkt = av_packet_alloc();
    if (pkt) {
        av_packet_unref(pkt);
    } else {
        qDebug() << "===初始化 pkt 失败===";
        return;
    }

    while (state->is_playing) {
        if (state->is_seeking) {
            int64_t seek_target = (int) state->start_time * 1000 * 1000;
            qDebug() << "===seek_target===" << seek_target;
            int index = av_find_default_stream_index(fmt_ctx);

            // 寻找最近的关键帧
            int video_seek_result = avformat_seek_file(fmt_ctx, -1,
                                                       INT64_MIN, seek_target,
                                                       INT64_MAX, 0); // 寻找最近的关键帧
            if (video_seek_result < 0) {
                qDebug() << "Error video_seek_result frame: " << av_err2str(video_seek_result);
                break;
            }

            state->is_seeking = false;
        }

        int ret = av_read_frame(fmt_ctx, pkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                qDebug() << "===av_read_frame === 结束 == ";
            } else {
                qDebug() << "===av_read_frame === 其他错误 == " << ret;
            }
            av_packet_unref(pkt);
            break;
        };


        if (pkt->stream_index == state->video_stream_index) {
            state->addVideoPacket(pkt);
        } else if (pkt->stream_index == state->audio_stream_index) {
            state->addAudioPacket(pkt);
        } else {

        }

        av_packet_unref(pkt);
    }
    qDebug() << "===VideoDecoder::readFrameLoop== 结束 =";

}


void VideoDecoder::changeVolume(int volume) {
// 设置音量，范围为0（静音）到MIX_MAX_VOLUME（最大音量，通常是128）
    Mix_VolumeMusic(MIX_MAX_VOLUME * volume / 100);
    Mix_Volume(2, MIX_MAX_VOLUME * volume / 100);
    qDebug() << "===changeVolume===" << volume;
}

VideoDecoder::~VideoDecoder() {

}

void VideoDecoder::onSoundOff() {
}


double VideoDecoder::getDuration() {
    return this->m_duration;
}

void VideoDecoder::changeProgress(int targetTimeSeconds) {


    qDebug() << "===暂停解码===";
    this->onStop();
    av_usleep(10000);
    DecodeState::getInstance()->start_time = targetTimeSeconds;
    DecodeState::getInstance()->is_seeking = true;


    qDebug() << "===重新开始了===";
    this->onStart();

    this->startRead();
}


bool VideoDecoder::onPlayAndPause(bool pause) {
    DecodeState::getInstance()->is_playing = pause;
    if (DecodeState::getInstance()->is_playing) {
        readFrameThread = new ReadFrameThread(this);
        QThreadPool::globalInstance()->start(readFrameThread);

    } else {
        readFrameThread = nullptr;
    }
    qDebug() << "===DecodeState::getInstance()->is_playing===" << DecodeState::getInstance()->is_playing;
    emit  playAndPause(DecodeState::getInstance()->is_playing);

    return true;

}

void VideoDecoder::onStart() {
    DecodeState::getInstance()->is_playing = true;

    decode_video->startDecode();
    decode_audio->startDecode();
    emit start();
}


void VideoDecoder::startRead() {
    readFrameThread = new ReadFrameThread(this);
    QThreadPool::globalInstance()->start(readFrameThread);
}

void VideoDecoder::onStop() {
    DecodeState::getInstance()->is_playing = false;
    DecodeState::getInstance()->is_decoding = false;
//     decode_video->stopDecode();
//     decode_audio->stopDecode();
    emit stop();
}




