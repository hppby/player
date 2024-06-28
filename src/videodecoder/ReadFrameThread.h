//
// Created by LOOG LS on 2024/6/27.
//

#ifndef PLAYER_READFRAMETHREAD_H
#define PLAYER_READFRAMETHREAD_H


#include <QObject>
#include <QRunnable>

class VideoDecoder;

class ReadFrameThread: public QObject, public QRunnable {
QOBJECT_H

public:
    explicit ReadFrameThread(VideoDecoder *parent = nullptr);
    ~ReadFrameThread() override;

    void run() override;

private:
    VideoDecoder *videoDecoder;

};


#endif //PLAYER_READFRAMETHREAD_H
