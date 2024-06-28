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


VideoDecoder::VideoDecoder(QWidget *parent, QLabel *playerLabel)
        : QObject(parent) {

    m_playView = playerLabel;
    // 初始化FFmpeg库
    avformat_network_init();
    this->isRunning = false;
}

bool VideoDecoder::openFile(QString fileName) {
    this->initDecode(fileName);

    this->startOrPause();

    isChangedWindowSize = false;
    return true;
}


bool VideoDecoder::initDecode(QString fileName) {
    QByteArray ba = fileName.toLocal8Bit();
    const char *c_filename = ba.data();

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

    connect(this, &VideoDecoder::start, decode_audio, &DecodeAudio::startOrPause);
    connect(this, &VideoDecoder::start, decode_video, &DecodeVideo::startOrPause);

    this->m_duration = (double) fmt_ctx->duration / AV_TIME_BASE;


    return true;
}

void VideoDecoder::readFrameLoop() {


    while (this->isRunning) {
        my_mutex.lock();
        AVPacket *pkt = av_packet_alloc();
        if (pkt) {
            av_packet_unref(pkt);
        } else {
            pkt = av_packet_alloc();
            continue;
        }
        if (av_read_frame(fmt_ctx, pkt) < 0) {
//            isEnd = true;
//            break;
            my_mutex.unlock();
            continue;
        };
        my_mutex.unlock();

        if (pkt->stream_index == decode_video->video_stream_index) {
            decode_video->addPacket(pkt);
        } else if (pkt->stream_index == decode_audio->audio_stream_index) {
            decode_audio->addPacket(pkt);
        } else {

        }
        av_packet_free(&pkt);

    }


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
    this->isRunning = false;


        qDebug() << "===target_time_seconds===" << targetTimeSeconds;

        // 寻找最近的关键帧
        int seek_result = av_seek_frame(fmt_ctx, decode_video->video_stream_index, targetTimeSeconds,
                                        AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);

        if (seek_result < 0) {
            fprintf(stderr, "Error seeking frame: %s\n", av_err2str(seek_result));
            return;
        }

        // 重置解码器
        avcodec_flush_buffers(decode_video->video_dec_ctx);
        avcodec_flush_buffers(decode_audio->audio_dec_ctx);
        this->isRunning = true;



}

bool VideoDecoder::stop() {
    this->isRunning = false;
    return false;
}

bool VideoDecoder::startOrPause() {
    if (isRunning) {
        this->isRunning = false;
        readFrameThread = nullptr;
    } else {
        this->isRunning = true;
        readFrameThread = new ReadFrameThread(this);
        QThreadPool::globalInstance()->start(readFrameThread);
    }

    emit  start(this->isRunning);

    return true;

}




