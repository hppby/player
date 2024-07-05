//
// Created by LOOG LS on 2024/6/26.
//

#include "DecodeVideo.h"
#include "VideoDecoder.h"
#include <QDebug>
#include "DecodeState.h"
#include "VideoPlayThread.h"


DecodeVideo::DecodeVideo(VideoDecoder *parent, AVFormatContext *fmtCtx) {
    this->video_decoder = parent;
//    this->packet_queue = new QQueue<AVPacket *>();
//    this->skip_duration = 0.0;
}

bool DecodeVideo::init() {
    bool ret = this->_init();
    if (!ret) {
        this->closeFile();
    }
    return ret;
}

bool DecodeVideo::_init() {
    AVFormatContext *fmt_ctx = DecodeState::getInstance()->fmt_ctx;
    int video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_index < 0) {
        qDebug() << "查找视频流--失败";
        return false;
    }
    DecodeState::getInstance()->video_stream_index = video_stream_index;
    DecodeState::getInstance()->video_stream = fmt_ctx->streams[video_stream_index];

    // 初始化视频解码器
    if (!this->initVideoDecoder()) return false;

    return true;
}


// 初始化音频解码器
bool DecodeVideo::initVideoDecoder() {
    const AVCodecParameters *codecpar = DecodeState::getInstance()->video_stream->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        qDebug() << QString("查找解码器--失败 (流类型: %1)").arg("视频");
        return false;
    }
    DecodeState::getInstance()->video_dec_ctx = avcodec_alloc_context3(codec);
    if (!(DecodeState::getInstance()->video_dec_ctx)) {
        qDebug() << QString("分配解码器上下文--失败 (流类型: %1)").arg("视频");
        return false;
    }
    if (avcodec_parameters_to_context(DecodeState::getInstance()->video_dec_ctx, codecpar) < 0) {
        qDebug() << QString("复制编解码参数到解码上下文--失败 (流类型: %1)").arg("视频");
        return false;
    }
    if (avcodec_open2(DecodeState::getInstance()->video_dec_ctx, codec, nullptr) < 0) {
        qDebug() << QString("打开解码器--失败 (流类型: %1)").arg("视频");
        return false;
    }

    // 初始化转换上下文
    DecodeState::getInstance()->video_sws_ctx = sws_getContext(DecodeState::getInstance()->video_dec_ctx->width,
                                                               DecodeState::getInstance()->video_dec_ctx->height,
                                                               DecodeState::getInstance()->video_dec_ctx->pix_fmt,
                                                               DecodeState::getInstance()->video_dec_ctx->width,
                                                               DecodeState::getInstance()->video_dec_ctx->height,
                                                               AV_PIX_FMT_RGB32,
                                                               SWS_BILINEAR, nullptr, nullptr, nullptr);
    return true;
}


QImage DecodeVideo::convertToQImage(AVFrame *src_frame, SwsContext *sws_ctx) {

    // 检查sws_ctx有效性，避免空指针调用
    if (!sws_ctx) {
        qDebug() << "SwsContext is null!";
        return QImage(); // 或者根据需要抛出异常
    }
    if (!src_frame) {
        qDebug() << "src_frame is null!";
        return QImage(); // 或者根据需要抛出异常
    }

    // 确保目标图像的大小与源帧匹配
    QImage dest(src_frame->width, src_frame->height, QImage::Format_RGB32);

    // 设置转换参数
    uint8_t *dst_data[4] = {dest.bits(), nullptr, nullptr, nullptr}; // RGB32格式只需第一个指针
    int dst_linesize[4]; // 其余为0，因为RGB32是平面格式
    dst_linesize[0] = static_cast<int>(dest.bytesPerLine()); // 显式类型转换
    dst_linesize[1] = 0;
    dst_linesize[2] = 0;
    dst_linesize[3] = 0;

    // 执行转换并检查是否成功
    int result = sws_scale(sws_ctx, src_frame->data, src_frame->linesize, 0, src_frame->height,
                           dst_data, dst_linesize);
    if (result <= 0) {
        qDebug() << "sws_scale failed with error code:" << result;
        return QImage(); // 或者根据需要处理错误
    }

    // 返回转换后的图像
    return dest;
}

void DecodeVideo::decodeVideoFrame(AVPacket *pkt) {
    DecodeState *state = DecodeState::getInstance();
    if (!state || !state->is_decoding) {
        return;
    }

    QWriteLocker locker(&decode_mutex);
    int ret = avcodec_send_packet(state->video_dec_ctx, pkt);
    if (ret < 0) {
        qDebug() << "avcodec_send_packet failed with error code: " << ret;
        return;
    }

    while (true) {
        AVFrame *videoFrame = av_frame_alloc();
        if (!videoFrame) {
            qDebug() << "Failed to allocate memory for AVFrame";
            return; // Exit loop if frame allocation fails
        }

        av_frame_unref(videoFrame);

        int receiveResult = avcodec_receive_frame(state->video_dec_ctx, videoFrame);
        if (receiveResult == AVERROR(EAGAIN) || receiveResult == AVERROR_EOF) {
            qDebug() << "===结束获取数据===";
            av_frame_free(&videoFrame);
            break;
        } else if (receiveResult < 0) {
            qDebug() << "avcodec_receive_frame failed with error code: " << receiveResult;
            av_frame_free(&videoFrame);
            break;
        }

        DecodeState::getInstance()->addVideoFrame(videoFrame);
    }

qDebug() << "===解码结束==1=";
}


void DecodeVideo::decodeLoop() {
    qDebug() << "===DecodeVideo::decodeLoop===开始==";
    double videoPts = 0.0;
    int64_t startTime = av_gettime();


    DecodeState *state = DecodeState::getInstance();

    while (DecodeState::getInstance()->is_decoding) {


        AVPacket *packet = state->getVideoPacket();
        if (packet) {
            // 计算当前时间
            videoPts = av_q2d(state->fmt_ctx->streams[state->video_stream_index]->time_base) * packet->pts;

            int64_t currentTime = (videoPts - state->start_time) * 1000 * 1000 + startTime;
            duration = videoPts;

            int64_t diff = currentTime - av_gettime();

            if (diff < 0) {
                av_packet_free(&packet);
                continue;
            }
            this->decodeVideoFrame(packet);
            av_packet_free(&packet);
        } else {
            av_usleep(1000);
        }
    }
    qDebug() << "===DecodeVideo::decodeLoop===结束==";
}

void DecodeVideo::playLoop() {

    double videoPts = 0.0;
    qDebug() << "===DecodeVideo::playLoop===开始==";

    int64_t startTime = av_gettime();

    DecodeState *state = DecodeState::getInstance();
    while (DecodeState::getInstance()->is_playing) {


        AVFrame *frame = state->getVideoFrame();

        if (frame) {


            // 计算当前时间
            videoPts = av_q2d(state->fmt_ctx->streams[state->video_stream_index]->time_base) * frame->pts;

            int64_t currentTime = (videoPts - state->start_time) * 1000 * 1000 + startTime;
            duration = videoPts;
            int64_t diff = currentTime - av_gettime();

            if (diff > 100 * 1000) {
                qDebug() << "===diff===" << diff;
                av_usleep(diff / 2);
            } else if (diff < -100 * 1000) {
                qDebug() << "===diff===" << diff;
                if (frame) {
                    av_frame_free(&frame);
                }
                continue;
            }

            QImage newFrame = this->convertToQImage(frame, state->video_sws_ctx);
            QPixmap pixmap = QPixmap::fromImage(newFrame);
            if (!pixmap.isNull()) {
                emit this->videoImageChanged(pixmap);
            }
            emit currentTimeChanged(state->start_time > videoPts ? state->start_time : videoPts);
            av_frame_free(&frame);
        } else {
            av_usleep(1000);
        }
    }
    qDebug() << "===DecodeVideo::decodeLoop===结束==";
}

void DecodeVideo::closeFile() {

    if (DecodeState::getInstance()->fmt_ctx) {
        avformat_close_input(&DecodeState::getInstance()->fmt_ctx);
        DecodeState::getInstance()->fmt_ctx = nullptr;
    }
    if (DecodeState::getInstance()->video_dec_ctx) {
        avcodec_free_context(&DecodeState::getInstance()->video_dec_ctx);
        DecodeState::getInstance()->video_dec_ctx = nullptr;
    }

    if (DecodeState::getInstance()->video_sws_ctx) {
        sws_freeContext(DecodeState::getInstance()->video_sws_ctx);
        DecodeState::getInstance()->video_sws_ctx = nullptr;
    }

}


void DecodeVideo::playAndPause(bool pause) {
    QMutexLocker locker(&start_or_pause_mutex);
    if (DecodeState::getInstance()->is_playing) {
        videoPlayThread = new VideoPlayThread(this);
        QThreadPool::globalInstance()->start(videoPlayThread);
    } else {
        DecodeState::getInstance()->start_time = duration;
        videoPlayThread = nullptr;
    }
}

DecodeVideo::~DecodeVideo() {
    this->closeFile();
}

void DecodeVideo::stop() {
    DecodeState::getInstance()->start_time = 0.0;
    videoDecoderThread = nullptr;
}

void DecodeVideo::start() {
    this->playAndPause(true);
}

void DecodeVideo::startDecode() {
    qDebug() << "===开始解码===";

    DecodeState::getInstance()->is_decoding = true;
    videoDecoderThread = new VideoDecoderThread(this);
    QThreadPool::globalInstance()->start(videoDecoderThread);
}

void DecodeVideo::stopDecode() {
    qDebug() << "===结束解码===";
    DecodeState::getInstance()->is_decoding = false;

}

