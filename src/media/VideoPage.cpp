//
// Created by LOOG LS on 2024/7/5.
//

#include "VideoPage.h"
#include "../db/SqlVideo.h"
#include "../network/HttpClient.h"
#include "../download/HandleMagnet.h"


VideoPage::VideoPage(QWidget *parent) {

    SqlVideo *sqlVideo = new SqlVideo(this);
    HttpClient("https://baidu.com").debug(true).success([](const QString &response) {
        qDebug() << "===response===" << response;
    }).fail([](const QString &error, int errorCode) {
        qDebug() << "===error===" << error;
        qDebug().noquote() << error << errorCode;
    }).get();

    HandleMagnet *handleMagnet = new HandleMagnet(this);
}

VideoPage::~VideoPage() {

}

void VideoPage::initUI() {

}
