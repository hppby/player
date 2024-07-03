//
// Created by LOOG LS on 2024/7/3.
//

#include "AudioPlayThread.h"
#include "DecodeAudio.h"

AudioPlayThread::AudioPlayThread(DecodeAudio *decode_audio) {
    this->decode_audio = decode_audio;
}

AudioPlayThread::~AudioPlayThread() {

}

void AudioPlayThread::run() {
    this->decode_audio->playLoop();
}


