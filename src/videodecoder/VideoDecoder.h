//
// Created by LOOG LS on 2024/6/11.
//

#ifndef PLAYER_VIDEODECODER_H
#define PLAYER_VIDEODECODER_H


#include <QWidget>
#include <QTimer>
#include <QImage>
#include <QLabel>
#include <QMutex>
#include <SDL2/SDL.h>
#include <QReadWriteLock>
#include <QThread>

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

class ReadFrameThread;
class DecodeVideo;
class DecodeAudio;

class VideoDecoder : public QObject {
Q_OBJECT
public:
    explicit VideoDecoder(QWidget *parent = nullptr, QLabel *playView = nullptr);
    bool openFile(QString fileName);

    void readFrameLoop();


    void changeVolume(int volume);

    void onSoundOff();

    double getDuration();

    void changeProgress(int targetTimeSeconds);

    void onStart();
    void onStop();
    bool onPlayAndPause(bool pause);

    QSize videoSize;

    bool isChangedWindowSize;

    ~VideoDecoder();

    void startRead();

signals:
    void videoImageChanged(QPixmap pixmap);
    void currentTimeChanged(double currentTime);

    void start();
    void stop();
    void playAndPause(bool pause);

private slots:

private:



    DecodeVideo *decode_video;
    DecodeAudio *decode_audio;

    ReadFrameThread *readFrameThread;

    AVFormatContext *fmt_ctx = nullptr;

    QMutex my_mutex;

    QMutex progress_mutex;

    int64_t startTime = 0.0;

    QImage m_frame;
    QTimer m_timer;

    QLabel *m_playView;

    double m_duration;
    double  m_currentTime;

    bool isChangedProgress;

    QTimer *m_progressTimer;



    bool initDecode(QString fileName);



    QReadWriteLock rwLock;



};


#endif //PLAYER_VIDEODECODER_H
