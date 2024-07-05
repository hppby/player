//
// Created by LOOG LS on 2024/7/1.
//

#include <QMutex>
#include <QCoreApplication>
#include "DecodeState.h"
#include <QtCore/qqueue.h>

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

    this->audio_packet_queue = new QQueue<AVPacket *>();
    this->video_packet_queue = new QQueue<AVPacket *>();

    this->video_frame_queue = new QQueue<AVFrame *>();
    this->audio_frame_queue = new QQueue<AVFrame *>();

}

void DecodeState::addVideoPacket(AVPacket *pkt) {
    AVPacket *pkt1 = av_packet_alloc();
    if (!pkt1) {
        av_packet_unref(pkt);
        return;
    }
    av_packet_move_ref(pkt1, pkt);
    QMutexLocker locker(&video_packet_queue_mutex);
    DecodeState::getInstance()->video_packet_queue->enqueue(pkt1);

}

void DecodeState::addAudioPacket(AVPacket *pkt) {
    AVPacket *pkt1 = av_packet_alloc();
    if (!pkt1) {
        av_packet_unref(pkt);
        return;
    }
    av_packet_move_ref(pkt1, pkt);
    QMutexLocker locker(&audio_packet_queue_mutex);
    DecodeState::getInstance()->audio_packet_queue->enqueue(pkt1);

}
void DecodeState::addVideoFrame(AVFrame *frame) {
    if (!DecodeState::getInstance()->is_decoding) return;
    QMutexLocker locker(&video_frame_queue_mutex);
    DecodeState::getInstance()->video_frame_queue->enqueue(frame);
}

void DecodeState::addAudioFrame(AVFrame *frame) {
    QMutexLocker locker(&audio_frame_queue_mutex);
    DecodeState::getInstance()->audio_frame_queue->enqueue(frame);
}

void DecodeState::clearQueue() {
    DecodeState::getInstance()->audio_frame_queue->clear();
    DecodeState::getInstance()->video_frame_queue->clear();
    DecodeState::getInstance()->audio_packet_queue->clear();
    DecodeState::getInstance()->video_packet_queue->clear();
}

AVPacket *DecodeState::getVideoPacket() {
    QMutexLocker locker(&video_packet_queue_mutex);
    return DecodeState::getInstance()->video_packet_queue->isEmpty() ? nullptr :  DecodeState::getInstance()->video_packet_queue->dequeue();
}

AVPacket *DecodeState::getAudioPacket() {
    QMutexLocker locker(&audio_packet_queue_mutex);
    return DecodeState::getInstance()->audio_packet_queue->isEmpty() ? nullptr : DecodeState::getInstance()->audio_packet_queue->dequeue();
}

AVFrame *DecodeState::getVideoFrame() {
    QMutexLocker locker(&video_frame_queue_mutex);
    return DecodeState::getInstance()->video_frame_queue->isEmpty() ? nullptr : DecodeState::getInstance()->video_frame_queue->dequeue();
}

AVFrame *DecodeState::getAudioFrame() {
    QMutexLocker locker(&audio_frame_queue_mutex);
    return DecodeState::getInstance()->audio_frame_queue->isEmpty() ? nullptr : DecodeState::getInstance()->audio_frame_queue->dequeue();
}


