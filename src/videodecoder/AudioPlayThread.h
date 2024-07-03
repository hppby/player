//
// Created by LOOG LS on 2024/7/3.
//

#ifndef PLAYER_AUDIOPLAYTHREAD_H
#define PLAYER_AUDIOPLAYTHREAD_H


#include <QtCore/QObject>
#include <QRunnable>

class DecodeAudio;

class AudioPlayThread : public QObject, public QRunnable {
    QOBJECT_H

public:
    AudioPlayThread(DecodeAudio *decode_audio);

    ~AudioPlayThread();

    void run() override;

private:
    DecodeAudio *decode_audio;

};


#endif //PLAYER_AUDIOPLAYTHREAD_H
