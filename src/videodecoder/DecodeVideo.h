//
// Created by LOOG LS on 2024/6/26.
//

#ifndef PLAYER_DECODEVIDEO_H
#define PLAYER_DECODEVIDEO_H


#include <QObject>
#include <QLabel>
#include <QMutex>
#include <QtCore/qqueue.h>
#include "VideoDecoderThread.h"

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

class VideoDecoder;
class VideoPlayThread;
class DecodeFrame;


class DecodeVideo : public QObject {
Q_OBJECT

public:
    explicit DecodeVideo(VideoDecoder *parent, AVFormatContext *fmtCtx);

    ~DecodeVideo();


//    AVFormatContext *fmt_ctx = nullptr;
//    AVStream *video_stream = nullptr;
//    AVCodecContext *video_dec_ctx = nullptr;
//    SwsContext *video_sws_ctx = nullptr;
//    int video_stream_index = -1;
//    bool is_playing = false;

    void start(double time);
    void playAndPause(bool pause);
    void stop();
    bool init();
    void decodeLoop();
    void playLoop();

    void decodeVideoFrame(DecodeFrame *frame);

    void startDecode();
    void stopDecode();
private:

    QMutex codec_mutex;

    VideoDecoder *video_decoder;
    VideoDecoderThread *videoDecoderThread = nullptr;
    VideoPlayThread *videoPlayThread = nullptr;

    QMutex decode_mutex;
    QMutex start_or_pause_mutex;

    double duration;


    bool _init();
    bool initVideoDecoder();

    QImage convertToQImage(AVFrame *src_frame, SwsContext *sws_ctx);



    void closeFile();

    void resetPacketQueue();

signals:
    void videoImageChanged(QPixmap pixmap);
    void currentTimeChanged(double currentTime);


};


#endif //PLAYER_DECODEVIDEO_H
