//
// Created by LOOG LS on 2024/6/21.
//

#ifndef PLAYER_VIDEODECODERTHREAD_H
#define PLAYER_VIDEODECODERTHREAD_H

#include <QRunnable>
#include <QThreadPool>


class VideoDecoder;

class VideoDecoderThread:public QObject, public QRunnable {
Q_OBJECT

public:
    explicit VideoDecoderThread(VideoDecoder *parent = nullptr);

private:
    VideoDecoder *videoDecoder;

protected:
     void run() override;

};


#endif //PLAYER_VIDEODECODERTHREAD_H
