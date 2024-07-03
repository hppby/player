//
// Created by LOOG LS on 2024/7/2.
//

#include "DecodeFrame.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/frame.h>
#include <libavutil/time.h>
#include <libavutil/channel_layout.h>
}

DecodeFrame::DecodeFrame(AVPacket *pkt) {
    this->pkt = pkt;
    this->pts = pkt->pts;
    this->duration = pkt->duration;
    this->pos = pkt->pos;
}

DecodeFrame::~DecodeFrame() {

}


