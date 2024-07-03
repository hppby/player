//
// Created by LOOG LS on 2024/7/2.
//

#ifndef PLAYER_VIDEOPLAYTHREAD_H
#define PLAYER_VIDEOPLAYTHREAD_H


#include <QObject>
#include <QRunnable>

class DecodeVideo;

class VideoPlayThread: public QObject, public QRunnable  {
QOBJECT_H

public:
    explicit VideoPlayThread(DecodeVideo *parent = nullptr);
    ~VideoPlayThread() override;
    void run() override;

private:
    DecodeVideo *decode_video;

};


#endif //PLAYER_VIDEOPLAYTHREAD_H
