//
// Created by LOOG LS on 2024/7/2.
//

#include "VideoPlayThread.h"
#include "DecodeVideo.h"

VideoPlayThread::VideoPlayThread(DecodeVideo *parent) {

    this->decode_video = parent;
}

VideoPlayThread::~VideoPlayThread() {

}

void VideoPlayThread::run() {
    this->decode_video->playLoop();
}
