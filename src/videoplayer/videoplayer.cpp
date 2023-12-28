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
#include "videoplayer.h"

//#include <>


Videoplayer::Videoplayer(QWidget *parent) : QWidget(parent), m_voice(50),
                                            currentFormattedTime("00:00") {
    this->setGeometry(100, 100, 1024, 600);
    this->setAttribute(Qt::WA_DeleteOnClose, true);
    this->setMouseTracking(true);
    isProgressMoving = false;
    m_isControlShowing = true;
    m_headerHeight = 30;
    m_controlHeight = 80;
    m_timer = new QTimer(this);
    hlayout = new QVBoxLayout(this);
    hlayout->setContentsMargins(0, 0, 0, 0);


    InitHeader();
    // 初始化播放器
    InitPlayer();

    InitControl();

    connect(m_timer,&QTimer::timeout,this,[=](){
        qDebug() << "timeout=====";
        headerContent->setFixedHeight(0);
        controlBox->setFixedHeight(0);
        m_isControlShowing=false;
        m_timer->stop();
    });

}

void Videoplayer::InitHeader() {
    headerContent = new QWidget(this);

    headerContent->setStyleSheet("background-color: rgb(242,156,177)");
    headerContent->setFixedHeight(m_headerHeight);
    hlayout->addWidget(headerContent);
    hlayout->addSpacing(0);
    m_titleLabel = new QLabel(headerContent);
    QHBoxLayout *titleOlayout = new QHBoxLayout(headerContent);
    titleOlayout->setContentsMargins(0, 0, 0, 0);
    titleOlayout->addWidget(m_titleLabel);
    m_titleLabel->setText("正在为大宝播放：《》");
    m_titleLabel->setAlignment(Qt::AlignCenter);

}

void Videoplayer::InitPlayer() {
    audiooutput = new QAudioOutput();
    player = new QMediaPlayer;
    player->setPlaybackRate(1.0);//默认1倍速播放

    QWidget *playerContent = new QWidget(this);

    QVBoxLayout *playerContentLayout = new QVBoxLayout(playerContent);

    videoWidget = new QVideoWidget(playerContent);
    QPushButton *playerContentBtn = new QPushButton(videoWidget);
    playerContentBtn->setFixedSize(111,555);
    hlayout->addWidget(videoWidget);
    hlayout->setStretch(1, 9);

    videoWidget->setAspectRatioMode(Qt::KeepAspectRatioByExpanding);//缩放适应videoWidget的大小
    player->setVideoOutput(videoWidget);//设置播放窗口
    player->setAudioOutput(audiooutput);//设置声音
    audiooutput->setVolume(m_voice);//初始音量为50

    connect(playerContentBtn, &QPushButton::clicked, [=]() {

        qDebug() << "QPushButton::pressed";
        showControlBox();
    });

}

void Videoplayer::InitControl() {

    controlBox = new QWidget(this);
    controlBox->setFixedHeight(m_controlHeight);
    hlayout->addWidget(controlBox);

    QVBoxLayout *controlLayout = new QVBoxLayout(controlBox);

    controlBox->setStyleSheet("background-color: rgb(242,156,177)");

    QWidget *progressBox = new QWidget(controlBox);
    controlLayout->addWidget(progressBox, 1);

    QHBoxLayout *progressLayout = new QHBoxLayout(progressBox);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    m_currentTimeLabel = new QLabel();
    m_currentTimeLabel->setFixedWidth(100);
    m_Progressslider = new QSlider(Qt::Horizontal);
    m_lastTimeLabel = new QLabel();
    m_lastTimeLabel->setFixedWidth(100);
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
    m_voiceslider->setValue(50);

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
        if (audiooutput->volume() > 0) {
            audiooutput->setVolume(0);
        } else {
            audiooutput->setVolume(m_voice);
        }
        showControlBox();
    });

    // 设置音量
    connect(m_voiceslider, &QSlider::valueChanged, [=](int voice) {
        m_voice = voice;
        audiooutput->setVolume((float) voice/(float) (m_voiceslider->width()));
        showControlBox();
    });

    // 设置暂停播放
    connect(m_playbutton, &QPushButton::clicked, [=]() {
        if (player->isPlaying()) {
            player->pause();
            ButtonStyleSet(m_playbutton, ":/resource/player-pause.png");
        } else {
            player->play();
            ButtonStyleSet(m_playbutton, ":/resource/player-play.png");
        }
    });

    // 设置播放进度
    connect(m_Progressslider, &QSlider::valueChanged, [=](int position) {
        m_progress = position;
        double progress = (double) m_progress / (double) (this->width()) * (double) player->duration();

        QString currentTime = QDateTime::fromMSecsSinceEpoch(progress, QTimeZone::utc()).toString("HH:mm:ss");
        m_currentTimeLabel->setText(currentTime);
        QString lastTime = QDateTime::fromMSecsSinceEpoch((player->duration() - progress), QTimeZone::utc()).toString("HH:mm:ss");
        m_lastTimeLabel->setText(lastTime);
    });

    // 设置播放进度
    connect(m_Progressslider, &QSlider::sliderPressed, [=]() {
        isProgressMoving = true;
        m_timer->stop();
        m_isControlShowing= true;
    });

    // 设置播放进度
    connect(m_Progressslider, &QSlider::sliderReleased, [=]() {
        double progress = (double) m_progress / (double) (this->width()) * (double) player->duration();
        player->setPosition(progress);
        isProgressMoving = false;
        showControlBox();
    });


    // 快进 10秒
    connect(m_aheadbutton, &QPushButton::clicked, [=]() {
        int position = player->position();
        player->setPosition(position + 10 * 1000);
        showControlBox();
    });
    // 快退 10秒
    connect(m_backbutton, &QPushButton::clicked, [=]() {
        int position = player->position();
        player->setPosition(position - 10 * 1000);
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
        if (!isProgressMoving  && player && m_Progressslider) {
            double progress = (double) position / (double) player->duration() * (double) (this->width());
            m_Progressslider->setValue(progress);
            QString currentTime = QDateTime::fromMSecsSinceEpoch(position, QTimeZone::utc()).toString("HH:mm:ss");
            m_currentTimeLabel->setText(currentTime);

            QString lastTime = QDateTime::fromMSecsSinceEpoch((player->duration() - position), QTimeZone::utc()).toString("HH:mm:ss");
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
        headerContent->setFixedHeight(m_headerHeight);
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
    player->setSource(QUrl::fromLocalFile(filename));
    player->play();
    showControlBox();
    m_titleLabel->setText(QString("正在为大宝播放：《%1》").arg(filename));
}

void Videoplayer::mouseMoveEvent(QMouseEvent *ev){

    qDebug() << "mouseMoveEvent";
    showControlBox();
}


Videoplayer::~Videoplayer() {


}
