//
// Created by LOOG LS on 2024/7/2.
//

#ifndef PLAYER_DECODEFRAME_H
#define PLAYER_DECODEFRAME_H


#include <QObject>

class AVPacket;
class AVFrame;

class DecodeFrame : public QObject {
QOBJECT_H

public:
    DecodeFrame(AVPacket *pkt);
    ~DecodeFrame();

    AVPacket *pkt;
    AVFrame *frame;

    int64_t duration;
    int64_t pts;
    int64_t pos;

    bool is_decoded;
    int decode_result; //解码结果 0 成功, 小于0 失败
    int index;


};


#endif //PLAYER_DECODEFRAME_H
