//
// Created by LOOG LS on 2024/7/1.
//

#ifndef PLAYER_DECODESTATE_H
#define PLAYER_DECODESTATE_H


#include <QtCore/QObject>

class AVFormatContext;
class AVStream;
class AVCodecContext;
class SwsContext;
class SwrContext;
class AVPacket;
class AVFrame;
class DecodeFrame;

class DecodeState : public QObject {
QOBJECT_H

public:


    static DecodeState* getInstance();
    AVFormatContext *fmt_ctx = nullptr;
    AVStream *video_stream = nullptr;
    AVCodecContext *video_dec_ctx = nullptr;
    SwsContext *video_sws_ctx = nullptr;
    int video_stream_index = -1;
    bool is_playing = false;

    bool is_decoding = false;

    AVCodecContext *audio_dec_ctx = nullptr;
    int audio_stream_index = -1;
    AVStream *audio_stream = nullptr;
    SwrContext *audio_swr_ctx = nullptr;


    double skip_duration;

    void addVideoPacket(AVPacket *pkt);
    void addAudioPacket(AVPacket *pkt);
    QQueue<AVPacket *> *video_packet_queue;
    QQueue<AVPacket *> *audio_packet_queue;

    QList<DecodeFrame *> *video_frame_queue;
    QList<AVFrame *> *audio_frame_queue;

    QMutex video_packet_queue_mutex;
    QMutex audio_packet_queue_mutex;

    void addVideoFrame(AVFrame *frame);
    void addAudioFrame(AVFrame *frame);

private:

    // 构造函数私有化，防止外部直接创建对象
    DecodeState();
    // 拷贝构造函数和赋值运算符私有化，防止拷贝
    DecodeState(const DecodeState&) = delete;
    DecodeState& operator=(const DecodeState&) = delete;

    static DecodeState* instance; // 单例实例指针
    static QMutex mutex; // 用于线程同步的互斥锁


};


#endif //PLAYER_DECODESTATE_H
