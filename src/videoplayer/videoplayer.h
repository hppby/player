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
#include "../videowidget/videowidget.h"


class Videoplayer : public QWidget {
Q_OBJECT

public:
    explicit Videoplayer(QWidget *parent = nullptr);
    // 播放文件
    void openFile(QString filename);


    ~Videoplayer() override;

private:


    VideoWidget *m_videoWidget;
    QTimer *m_progresss_timer;

    QTimer *m_timer;
    QLabel *m_titleLabel;

    QLabel *m_playView;

    QWidget *headerContent;
    QWidget *controlBox;

    QMediaPlayer *player;
    QVideoWidget *videoWidget;
    QAudioOutput *audiooutput;

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


    int m_voice;
    QRect m_rect;
    bool isProgressMoving;
    int m_progress;
    bool m_isControlShowing;

    int m_headerHeight;
    int m_controlHeight;

    QString totalFormattedTime;
    QString currentFormattedTime;

    // 初始化播放器
    void InitPlayer();

    // 初始化播放组件
    void InitControl();

    // 初始化ffemg组件
//    int playvideo(QString videoPath);

    // 初始化页头
    void InitHeader();

    // 设置按钮图标
    void ButtonStyleSet(QPushButton *button, QString IconPath);

//private slots:
    void showControlBox();

 public:
    //重写虚函数
    //鼠标移动事件

public slots:
    void videoImagehandler(QImage *image);
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};


#endif //PLAYER_VIDEOPLAYER_H
