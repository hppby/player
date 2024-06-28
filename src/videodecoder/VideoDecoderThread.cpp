//
// Created by LOOG LS on 2024/6/21.
//

#include "VideoDecoderThread.h"
#include <QDebug>
#include "DecodeVideo.h"

VideoDecoderThread::VideoDecoderThread(DecodeVideo *parent) {
    this->decode_video = parent;
}


void VideoDecoderThread::run() {

    this->decode_video->decodeLoop();

}


