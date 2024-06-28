//
// Created by LOOG LS on 2024/6/26.
//

#include "AudioDecoderThread.h"
#include "DecodeAudio.h"

AudioDecoderThread::AudioDecoderThread(DecodeAudio *parent) {
    this->decode_audio = parent;
}


void AudioDecoderThread::run() {

    this->decode_audio->decodeLoop();

}
