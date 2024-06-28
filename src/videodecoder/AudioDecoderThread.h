//
// Created by LOOG LS on 2024/6/26.
//

#ifndef PLAYER_AUDIODECODERTHREAD_H
#define PLAYER_AUDIODECODERTHREAD_H


#include <QObject>
#include <QRunnable>

class DecodeAudio;

class AudioDecoderThread: public QObject, public QRunnable {
QOBJECT_H
public:
    explicit AudioDecoderThread(DecodeAudio *parent = nullptr);

private:
    DecodeAudio *decode_audio;


protected:
    void run() override;

};


#endif //PLAYER_AUDIODECODERTHREAD_H
