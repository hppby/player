//
// Created by LOOG LS on 2024/6/21.
//

#include "VideoDecoderThread.h"
#include <QDebug>
#include "VideoDecoder.h"

VideoDecoderThread::VideoDecoderThread(VideoDecoder *parent) {
    this->videoDecoder = parent;
}


void VideoDecoderThread::run() {
qDebug() << "Starting task ";

    this->videoDecoder->decodeLoop();
// Do some work
qDebug() << "Finished task " ;
}


