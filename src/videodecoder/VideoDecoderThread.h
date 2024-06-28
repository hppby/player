//
// Created by LOOG LS on 2024/6/21.
//

#ifndef PLAYER_VIDEODECODERTHREAD_H
#define PLAYER_VIDEODECODERTHREAD_H

#include <QRunnable>
#include <QThreadPool>


class DecodeVideo;

class VideoDecoderThread:public QObject, public QRunnable {
Q_OBJECT

public:
    explicit VideoDecoderThread(DecodeVideo *parent = nullptr);

private:
    DecodeVideo *decode_video;


protected:
     void run() override;

};


#endif //PLAYER_VIDEODECODERTHREAD_H
