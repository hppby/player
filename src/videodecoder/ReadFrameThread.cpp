//
// Created by LOOG LS on 2024/6/27.
//

#include "ReadFrameThread.h"
#include "VideoDecoder.h"


ReadFrameThread::ReadFrameThread(VideoDecoder *parent) {
    this->videoDecoder = parent;
}

void ReadFrameThread::run() {

    this->videoDecoder->readFrameLoop();
}

ReadFrameThread::~ReadFrameThread() {

}
