//
// Created by LOOG LS on 2024/6/11.
//

#ifndef PLAYER_VIDEOWIDGET_H
#define PLAYER_VIDEOWIDGET_H


#include <QWidget>
#include <QTimer>
#include <QImage>
#include <QLabel>
#include <QMutex>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>

#include <libavutil/frame.h>
}

class VideoWidget : public QWidget {
Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = nullptr, QLabel *playView = nullptr);
    bool openFile(QString fileName);

    // true: 播放，false: 暂停
    bool startOrPause();

    void changeVolume(int volume);

    double m_duration;
    double  m_currentTime;

    ~VideoWidget();



private slots:

private:
    QImage m_frame;
    QTimer m_timer;

    QLabel *m_playView;

    bool m_exitFlag = false;
    bool m_playing = false;
    bool isRunning;

    void decodeLoop();

    bool initDecode(QString fileName);
    bool initDecoder(int streamIndex, AVCodecContext **ctx);
    bool fetchStreamInfo();
    bool openInputFile(const char *filename);


    void decodeVideoFrame(AVPacket &pkt, AVFrame *&decodedFrame, AVCodecContext *&dec_ctx, QMutex &mutex, SwsContext *&sws_ctx,
                          QLabel *&m_playView);

    QImage convertToQImage(AVFrame *src_frame, SwsContext *sws_ctx);


    void decodeAudioFrame(AVPacket &pkt, AVFrame *&audioFrame, AVCodecContext *&audio_dec_ctx);

    void processAudioFrame(AVFrame* frame, AVCodecContext *&audio_dec_ctx);
    SwrContext* initSwrContext(AVCodecContext *&audio_dec_ctx);

};


#endif //PLAYER_VIDEOWIDGET_H
