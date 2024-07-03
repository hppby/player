//
// Created by LOOG LS on 2024/7/1.
//

#include <QMutex>
#include <QCoreApplication>
#include "DecodeState.h"
#include <QtCore/qqueue.h>
#include "DecodeFrame.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/frame.h>
#include <libavutil/time.h>
#include <libavutil/channel_layout.h>
}

DecodeState *DecodeState::instance = nullptr;
QMutex DecodeState::mutex;

// 实现单例的getInstance方法
DecodeState *DecodeState::getInstance() {
    QMutexLocker locker(&mutex); // 在访问前锁定互斥锁
    if (instance == nullptr) {
        instance = new DecodeState(); // 确保只创建一次实例
    }
    return instance;
}

// 构造函数定义，可以在这里进行初始化操作
DecodeState::DecodeState() {
    // 初始化逻辑...
    this->video_packet_queue = new QQueue<AVPacket *>();
    this->audio_packet_queue = new QQueue<AVPacket *>();

    this->video_frame_queue = new QList<DecodeFrame *>();
    this->audio_frame_queue = new QList<AVFrame *>();

    this->skip_duration = 0.0;
}

void DecodeState::addVideoPacket(AVPacket *pkt) {


//    QMutexLocker locker(&video_packet_queue_mutex);

    DecodeFrame *frame = new DecodeFrame(pkt);
    frame->index = DecodeState::getInstance()->video_frame_queue->size();
    DecodeState::getInstance()->video_frame_queue->append(frame);

}

void DecodeState::addAudioPacket(AVPacket *pkt) {
    if (!DecodeState::getInstance()->is_playing) return;

    AVPacket *pkt1 = av_packet_alloc();
    if (!pkt1) {
        av_packet_unref(pkt1);
    }
    pkt1 = av_packet_clone(pkt);

    QMutexLocker locker(&audio_packet_queue_mutex);
    DecodeState::getInstance()->audio_packet_queue->enqueue(pkt1);
    av_packet_unref(pkt);
}
void DecodeState::addVideoFrame(AVFrame *frame) {
    if (!DecodeState::getInstance()->is_playing) return;

    QMutexLocker locker(&video_packet_queue_mutex);
//    DecodeState::getInstance()->video_frame_queue->append(frame);
}

void DecodeState::addAudioFrame(AVFrame *frame) {
    if (!DecodeState::getInstance()->is_playing) return;

    QMutexLocker locker(&audio_packet_queue_mutex);
    DecodeState::getInstance()->audio_frame_queue->append(frame);
}


