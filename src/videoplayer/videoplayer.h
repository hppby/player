//
// Created by LOOG LS on 2023/12/19.
//

#ifndef PLAYER_VIDEOPLAYER_H
#define PLAYER_VIDEOPLAYER_H

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


class Videoplayer : public QWidget {
Q_OBJECT

public:
    explicit Videoplayer(QWidget *parent = nullptr);
    // 播放文件
    void openFile(QString filename);

    ~Videoplayer() override;

private:

    QTimer *m_timer;
    QLabel *m_titleLabel;

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


    QVBoxLayout *hlayout;

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

    // 初始化页头
    void InitHeader();

    // 设置按钮图标
    void ButtonStyleSet(QPushButton *button, QString IconPath);

//private slots:
    void showControlBox();


 public:
    //重写虚函数
    //鼠标移动事件
    virtual void mouseMoveEvent(QMouseEvent *event);

};


#endif //PLAYER_VIDEOPLAYER_H
