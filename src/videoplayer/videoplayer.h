//
// Created by LOOG LS on 2023/12/19.
//



#ifndef PLAYER_VIDEOPLAYER_H
#define PLAYER_VIDEOPLAYER_H

//包含ffmpeg相关头文件，并告诉编译器以C语言方式处理
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avstring.h>
#include <libavutil/mathematics.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libavutil/dict.h>
#include <libavutil/parseutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/avassert.h>
#include <libavutil/time.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavcodec/avfft.h>
#include <libswresample/swresample.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
}

#include <QWidget>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QPushButton>
#include <QSlider>
#include <QHBoxLayout>
#include <QLabel>
#include <QTime>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QGridLayout>
#include <QResizeEvent>
#include "../videodecoder/VideoDecoder.h"


class Videoplayer : public QWidget {
Q_OBJECT

public:
    explicit Videoplayer(QWidget *parent = nullptr);
    // 播放文件
    void openFile(QString filename);


    ~Videoplayer() override;

private:


    VideoDecoder *video_decoder;

    QTimer *m_timer;

    QLabel *m_playView;

    QWidget *controlBox;

    QMediaPlayer *player;

    QPushButton *m_voicebutton;
    QSlider *m_voiceslider;
    QSlider *m_Progressslider;
    QLabel *m_currentTimeLabel;
    QLabel *m_lastTimeLabel;


    QPushButton *m_backbutton;
    QPushButton *m_playbutton;
    QPushButton *m_aheadbutton;
    QPushButton *m_fullscreen;

    QString m_resource;


    double current_time;
    int m_voice;
    QRect m_rect;
    bool isProgressMoving;
    int m_progress;
    bool m_isControlShowing;

    bool is_playing;
    int m_controlHeight;


    // 初始化播放器
    void InitPlayer();

    // 初始化播放组件
    void InitControl();


    // 设置按钮图标
    void ButtonStyleSet(QPushButton *button, QString IconPath);

//private slots:
    void showControlBox();

    QString secondsToHms(int seconds);

    void progresssValueChanged();
    void handleProgressChanged(int addProgress);

 public:
    //重写虚函数
    //鼠标移动事件

public slots:
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};


#endif //PLAYER_VIDEOPLAYER_H
