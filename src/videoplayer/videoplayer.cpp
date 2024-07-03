//
// Created by LOOG LS on 2023/12/19.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Videoplayer.h" resolved



#include <QVBoxLayout>
#include <QIcon>
#include <QDebug>
#include <QTimer>
#include <QPalette>
#include <QApplication>
#include <QScreen>
#include <QBoxLayout>
#include <QLabel>
#include <QTimeZone>


#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>

#include "videoplayer.h"


Videoplayer::Videoplayer(QWidget *parent) : QWidget(parent), m_voice(50) {

    this->setGeometry(100, 100, 1024, 600);
    this->setAttribute(Qt::WA_DeleteOnClose, true);
    this->setMouseTracking(true);
    isProgressMoving = false;
    m_isControlShowing = true;
    is_playing = true;
    m_controlHeight = 80;
    m_timer = new QTimer(this);


    // 初始化播放器
    InitPlayer();

    InitControl();

    connect(m_timer, &QTimer::timeout, this, [=]() {
        qDebug() << "timeout=====";
//        headerContent->setFixedHeight(0);
//        controlBox->setFixedHeight(0);
        m_isControlShowing = false;
        m_timer->stop();
    });

}

QString Videoplayer::secondsToHms(int seconds) {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;

    QString formattedTime = QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));

    return formattedTime;
}




void Videoplayer::InitPlayer() {
    m_playView = new QLabel(this);
    m_playView->setGeometry(0, 0, this->width(), this->height());
    m_playView->setStyleSheet("background-color: '#000000'");
    m_playView->setScaledContents(true);
    video_decoder = new VideoDecoder(this, m_playView);

    connect(video_decoder, &VideoDecoder::currentTimeChanged, this, [=](double currentTime){
        this->current_time = currentTime;
        this->m_currentTimeLabel ->setText(this->secondsToHms((int) currentTime));
        int totalTime = (int) this->video_decoder->getDuration();
        int lastTime =totalTime - ((int) currentTime);
        m_lastTimeLabel ->setText(this->secondsToHms(lastTime));

        this->progresssValueChanged();
    });

    connect(video_decoder, &VideoDecoder::videoImageChanged, this, [=](QPixmap pixmap ){
        this->m_playView->setPixmap(pixmap);
    });

}



void Videoplayer::InitControl() {

    controlBox = new QWidget(this);
    controlBox->setGeometry(0, this->height() - m_controlHeight, this->width(), m_controlHeight);

    QVBoxLayout *controlLayout = new QVBoxLayout(controlBox);
    controlBox->setStyleSheet("background-color: rgb(242,156,177)");
    QWidget *progressBox = new QWidget(controlBox);
    controlLayout->addWidget(progressBox, 1);

    QHBoxLayout *progressLayout = new QHBoxLayout(progressBox);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    m_currentTimeLabel = new QLabel(QString("00:00:00"));
    m_Progressslider = new QSlider(Qt::Horizontal);
    m_lastTimeLabel = new QLabel(QString("00:00:00"));
    m_lastTimeLabel->setAlignment(Qt::AlignRight);

    progressLayout->addWidget(m_currentTimeLabel);
    progressLayout->addWidget(m_Progressslider, 1);
    progressLayout->addWidget(m_lastTimeLabel);
    m_Progressslider->setValue(0);

    // 创建控制按钮
    QWidget *controlContent = new QWidget(controlBox);
    controlLayout->addWidget(controlContent, 3);

    QHBoxLayout *contentLayout = new QHBoxLayout(controlContent);
    contentLayout->setContentsMargins(20, 0, 20, 0);
    /**
     * 静音控制按钮
     */
    m_voicebutton = new QPushButton();
    ButtonStyleSet(m_voicebutton, ":/resource/player-play.png");

    // 声音控制滑块
    m_voiceslider = new QSlider(Qt::Horizontal);
    m_voiceslider->setRange(0, 100);
    m_voiceslider->setValue(100);

    /**
     * 播放暂停按钮
     */
    m_playbutton = new QPushButton();
    ButtonStyleSet(m_playbutton, ":/resource/player-play.png");

    /**
     * 倒退按钮
     */
    m_backbutton = new QPushButton();
    ButtonStyleSet(m_backbutton, ":/resource/arrow-left.png");

    /**
     * 前进按钮
     */
    m_aheadbutton = new QPushButton();
    ButtonStyleSet(m_aheadbutton, ":/resource/arrow-right.png");

    /**
     * 全屏按钮
     */
    m_fullscreen = new QPushButton();
    ButtonStyleSet(m_fullscreen, ":/resource/fullscreen-expand.png");

    contentLayout->addWidget(m_backbutton);
    contentLayout->addWidget(m_playbutton);
    contentLayout->addWidget(m_aheadbutton);
    contentLayout->addStretch();
    contentLayout->addWidget(m_voicebutton);
    contentLayout->addWidget(m_voiceslider);
    contentLayout->addStretch();
    contentLayout->addWidget(m_fullscreen);


    // 设置静音按钮
    connect(m_voicebutton, &QPushButton::clicked, [=]() {
        this->video_decoder->onSoundOff();
    });

    // 设置音量
    connect(m_voiceslider, &QSlider::valueChanged, [=](int voice) {
        m_voice = voice;
        video_decoder->changeVolume(m_voice);
        showControlBox();
    });

    // 设置暂停播放
    connect(m_playbutton, &QPushButton::clicked, [=]() {
        is_playing = !is_playing;
         video_decoder->onPlayAndPause(is_playing);
       if (is_playing) {
           ButtonStyleSet(m_playbutton, ":/resource/player-play.png");
       } else {
           ButtonStyleSet(m_playbutton, ":/resource/player-pause.png");
       }
    });

    // 设置播放进度
    connect(m_Progressslider, &QSlider::valueChanged, [=](int position) {
        m_progress = position;
    });

    // 设置播放进度
    connect(m_Progressslider, &QSlider::sliderPressed, [=]() {
        isProgressMoving = true;
        m_timer->stop();
        m_isControlShowing = true;
    });

    // 设置播放进度
    connect(m_Progressslider, &QSlider::sliderReleased, [=]() {
        isProgressMoving = false;
        showControlBox();
        this->handleProgressChanged(0);
    });


    // 快进 10秒
    connect(m_aheadbutton, &QPushButton::clicked, [=]() {
        this->handleProgressChanged(5);
        showControlBox();
    });
    // 快退 10秒
    connect(m_backbutton, &QPushButton::clicked, [=]() {
        this->handleProgressChanged(-5);
        showControlBox();
    });

    // 全屏
    connect(m_fullscreen, &QPushButton::clicked, [=]() {
        if (this->isFullScreen()) {
            this->showNormal();
            this->setGeometry(m_rect);
            ButtonStyleSet(m_fullscreen, ":/resource/fullscreen-expand.png");
        } else {
            m_rect = this->geometry();
            this->showFullScreen();
            QScreen *screen = QGuiApplication::primaryScreen();
            QRect screenRect = screen->geometry();
            this->setGeometry(screenRect);
            ButtonStyleSet(m_fullscreen, ":/resource/fullscreen-shrink.png");

            m_Progressslider->setRange(0, screen->geometry().width());//设置进度条范围

        }

        showControlBox();
    });

    // 播放监听
    connect(player, &QMediaPlayer::positionChanged, [=](int position) {
        if (!isProgressMoving && player && m_Progressslider) {
            double progress = (double) position / (double) player->duration() * (double) (this->width());
            m_Progressslider->setValue(progress);
            QString currentTime = QDateTime::fromMSecsSinceEpoch(position, QTimeZone::utc()).toString("HH:mm:ss");
            m_currentTimeLabel->setText(currentTime);

            QString lastTime = QDateTime::fromMSecsSinceEpoch((player->duration() - position),
                                                              QTimeZone::utc()).toString("HH:mm:ss");
            m_lastTimeLabel->setText(lastTime);
        }
    });
    // 播放监听
    connect(player, &QMediaPlayer::durationChanged, [=](int duration) {
        m_Progressslider->setRange(0, this->width());

    });

}

void Videoplayer::showControlBox() {
    if (m_isControlShowing) {

        if (m_timer->isActive()) {
            m_timer->stop();
        }

    } else {
        qDebug() << "ControlShowing=====";
        m_isControlShowing = true;
        controlBox->setFixedHeight(m_controlHeight);

    }
    m_timer->setInterval(5000);
    m_timer->start();
}


void Videoplayer::ButtonStyleSet(QPushButton *button, QString IconPath) {
    // 设置图像
    button->setIcon(QIcon(IconPath));
    button->setFlat(true);//去除边框
    button->setStyleSheet(
            "QPushButton:hover {background-color: grey;} QPushButton:pressed {background-color: darkGrey;}");
}

void Videoplayer::openFile(QString filename) {
    setWindowTitle(filename);
    bool isOpen = video_decoder->openFile(filename);

}

void Videoplayer::mouseMoveEvent(QMouseEvent *ev) {
    showControlBox();
}


void Videoplayer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event); // 调用基类实现
    // 在这里添加你的代码，比如重新布局内部控件
    qDebug() << "窗口大小已更改: " << event->size();

    this->video_decoder->videoSize = this->size();

    this->m_playView->resize(event->size().width(), event->size().height());

    this->controlBox->setGeometry(0, this->height() - m_controlHeight, this->width(), m_controlHeight);

    this->progresssValueChanged();
}



Videoplayer::~Videoplayer() {


}

// 更新进度条数据
void Videoplayer::progresssValueChanged() {

    if (isProgressMoving) return;

     double totalTime = this->video_decoder->getDuration();
    double currentTime = this->current_time;

    int fullWidth = this->m_Progressslider->width();
    this->m_Progressslider->setRange(0, fullWidth);
    if (currentTime > 0) {
        int progress = (int) (fullWidth * currentTime / totalTime);
        this->m_Progressslider->setValue(progress);
    }
}

void Videoplayer::handleProgressChanged(int addProgress) {
    int fullWidth = this->m_Progressslider->width();
    int progress = (int) this->m_progress;
    double totalTime = this->video_decoder->getDuration();
    if (progress > 0) {
        int64_t currentTime =(int64_t) totalTime * progress / fullWidth;
        if (currentTime + addProgress > 0) {
        this->video_decoder->changeProgress(currentTime + addProgress);
        } else {
            this->video_decoder->changeProgress(0);
        }
    }
}


