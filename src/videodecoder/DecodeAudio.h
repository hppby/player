//
// Created by LOOG LS on 2024/6/26.
//

#ifndef PLAYER_DECODEAUDIO_H
#define PLAYER_DECODEAUDIO_H

#include <QObject>
#include <SDL2/SDL.h>
#include <QMutex>
#include <QtCore/qqueue.h>


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

class AudioDecoderThread;
class DecodeFrame;
class AudioPlayThread;

class DecodeAudio : public QObject {
Q_OBJECT
public:

    DecodeAudio(AVFormatContext *fmtCtx = nullptr);
    ~DecodeAudio();



//    AVFormatContext *fmt_ctx = nullptr;
//    AVCodecContext *audio_dec_ctx = nullptr;
//    int audio_stream_index = -1;
//    bool is_playing = false;
//    AVStream *audio_stream = nullptr;
//    SwrContext *audio_swr_ctx = nullptr;

    bool init();
    bool start(double time);
    bool playAndPause(bool pause);
    void stop();
    void decodeLoop();
    void playLoop();

    void decodeAudioFrame(DecodeFrame *decode_frame);


    void startDecode();
    void stopDecode();

private:

    AudioDecoderThread *audioDecoderThread = nullptr;
    AudioPlayThread *audioPlayThread = nullptr;

    double duration;

    // 音频设备
    SDL_AudioDeviceID audio_device_id;
    // 设置音频参数
    SDL_AudioSpec desired_spec;

    QMutex my_mutex;
    QMutex decode_mutex;

    bool _init();
    bool initAudioDecoder();
    bool initSDL();

    void processAudioFrame(AVFrame *frame);

    void closeFile();

};


#endif //PLAYER_DECODEAUDIO_H
