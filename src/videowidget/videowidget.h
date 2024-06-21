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
}

class VideoWidget : public QWidget {
Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = nullptr, QLabel *playView = nullptr);
    bool openFile(QString fileName);

    // true: 播放，false: 暂停
    bool startOrPause();

    bool stop();

    void changeVolume(int volume);

    void onSoundOff();

  double getCurrentTime() const;
  double getDuration() const;

    void changePlaybackProgress(int64_t target_time_seconds);


    QSize videoSize;

    bool isChangedWindowSize;

    ~VideoWidget();


signals:
    void videoImageChanged(QPixmap pixmap);

private slots:

private:
    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *video_dec_ctx = nullptr;
    AVCodecContext *audio_dec_ctx = nullptr;

    AVStream *video_stream = nullptr;
    AVStream *audio_stream = nullptr;

    int video_stream_index = -1;
    int audio_stream_index = -1;


    SwsContext *video_sws_ctx = nullptr;
    SwrContext *audio_swr_ctx = nullptr;

    QImage m_frame;
    QTimer m_timer;

    QLabel *m_playView;

    double m_duration;
    double  m_currentTime;

    bool isRunning;

    void decodeLoop();

    bool initDecode(QString fileName);
    bool initDecoder(int streamIndex, AVCodecContext **ctx);
    bool fetchStreamInfo();
    bool openInputFile(const char *filename);

    void decodeVideoFrame(AVPacket &pkt, AVFrame *decodedFrame);

    QImage convertToQImage(AVFrame *src_frame, SwsContext *sws_ctx);


    void decodeAudioFrame(AVPacket &pkt, AVFrame *audioFrame);

    QReadWriteLock rwLock;
    void processAudioFrame(AVFrame* frame);
    SwrContext* initSwrContext(AVCodecContext *audio_dec_ctx);


    bool initSDL();

    // 音频设备
    SDL_AudioDeviceID audioDeviceId;
    // 音频FIFO
    AVAudioFifo *audioFifo;
    // 设置音频参数
    SDL_AudioSpec desiredSpec;
//    void audio_callback(void* userdata, Uint8* stream, int len);

    void closeFile();



};


#endif //PLAYER_VIDEOWIDGET_H
