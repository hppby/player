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

    this->onStart(0);

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
//    connect(this, &VideoDecoder::start, decode_audio, &DecodeAudio::start);
    connect(this, &VideoDecoder::start, decode_video, &DecodeVideo::start);

    // 播放 / 暂停
    connect(this, &VideoDecoder::stop, decode_audio, &DecodeAudio::stop);
    connect(this, &VideoDecoder::stop, decode_video, &DecodeVideo::stop);

    // 停止
    connect(this, &VideoDecoder::playAndPause, decode_audio, &DecodeAudio::playAndPause);
    connect(this, &VideoDecoder::playAndPause, decode_video, &DecodeVideo::playAndPause);

    this->m_duration = (double) fmt_ctx->duration / AV_TIME_BASE;


    return true;
}

void VideoDecoder::readFrameLoop() {

    qDebug() << "===VideoDecoder::readFrameLoop== 开始 =";
    DecodeState *state = DecodeState::getInstance();
    while (state->is_playing) {
        AVPacket *pkt = av_packet_alloc();
        if (pkt) {
            av_packet_unref(pkt);
        } else {
            continue;
        }
        int ret = av_read_frame(fmt_ctx, pkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                qDebug() << "===av_read_frame === 结束 == ";
            } else {
                qDebug() << "===av_read_frame === 其他错误 == " << ret;
            }
            av_packet_free(&pkt);
            break;
        };

        if (pkt->stream_index == state->video_stream_index) {
            state->addVideoPacket(pkt);
        } else if (pkt->stream_index == state->audio_stream_index) {
//            DecodeState::getInstance()->addAudioPacket(pkt);
        } else {

        }

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

void VideoDecoder::changeProgress(int targetTimeSeconds1) {

qDebug() << "===暂停解码===";
    this->onStop();
    av_usleep(100000);
    int64_t targetTimeSeconds = targetTimeSeconds1 * 1000;
    qDebug() << "===target_time_seconds===" << targetTimeSeconds;



    // 寻找最近的关键帧
    int video_seek_result = avformat_seek_file(fmt_ctx, DecodeState::getInstance()->video_stream_index,
                                               targetTimeSeconds - 2, targetTimeSeconds,
                                               targetTimeSeconds + 2, AVSEEK_FLAG_BACKWARD); // 寻找最近的关键帧
    if (video_seek_result < 0) {
        qDebug() << "Error video_seek_result frame: " << av_err2str(video_seek_result);
    }


    av_usleep(1000);
    qDebug() << "===重新开始了===" ;
    this->onStart(targetTimeSeconds1 * 1.0);

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

void VideoDecoder::onStart(double time) {
    DecodeState::getInstance()->is_playing = true;
    DecodeState::getInstance()->skip_duration = time;

    decode_video->startDecode();
    emit start(time);
}



void VideoDecoder::startRead() {
    readFrameThread = new ReadFrameThread(this);
    QThreadPool::globalInstance()->start(readFrameThread);
}

 void VideoDecoder::onStop() {
     decode_video->stopDecode();
    DecodeState::getInstance()->is_playing = false;
    emit stop();
}




