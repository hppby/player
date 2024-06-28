//
// Created by LOOG LS on 2024/6/26.
//

#include "DecodeVideo.h"
#include "VideoDecoder.h"
#include <QDebug>

DecodeVideo::DecodeVideo(VideoDecoder *parent, AVFormatContext *fmtCtx) {
    this->video_decoder = parent;
    this->fmt_ctx = fmtCtx;
    this->packet_queue = new QQueue<AVPacket *>();
    this->is_playing = false;
    this->skip_duration = 0.0;
}

bool DecodeVideo::init() {
    bool ret = this->_init();
    if (!ret) {
        this->closeFile();
    }
    return ret;
}

bool DecodeVideo::_init() {
    video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_index < 0) {
        qDebug() << "查找视频流--失败";
        return false;
    }
    video_stream = fmt_ctx->streams[video_stream_index];

    // 初始化视频解码器
    if (!this->initVideoDecoder()) return false;

    return true;
}


// 初始化音频解码器
bool DecodeVideo::initVideoDecoder() {
    const AVCodecParameters *codecpar = video_stream->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        qDebug() << QString("查找解码器--失败 (流类型: %1)").arg("视频");
        return false;
    }
    video_dec_ctx = avcodec_alloc_context3(codec);
    if (!(video_dec_ctx)) {
        qDebug() << QString("分配解码器上下文--失败 (流类型: %1)").arg("视频");
        return false;
    }
    if (avcodec_parameters_to_context(video_dec_ctx, codecpar) < 0) {
        qDebug() << QString("复制编解码参数到解码上下文--失败 (流类型: %1)").arg("视频");
        return false;
    }
    if (avcodec_open2(video_dec_ctx, codec, nullptr) < 0) {
        qDebug() << QString("打开解码器--失败 (流类型: %1)").arg("视频");
        return false;
    }

    // 初始化转换上下文
    video_sws_ctx = sws_getContext(video_dec_ctx->width, video_dec_ctx->height,
                                   video_dec_ctx->pix_fmt,
                                   video_dec_ctx->width, video_dec_ctx->height,
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

    if (!this->is_playing) {
        return;
    }

    int ret = avcodec_send_packet(video_dec_ctx, pkt);

    if (ret < 0) {
        qDebug() << "avcodec_send_packet faild";
        return;
    }

    AVFrame *videoFrame = av_frame_alloc();
    av_frame_unref(videoFrame);
    ret = avcodec_receive_frame(video_dec_ctx, videoFrame);

    if (ret < 0) {
        qDebug() << "avcodec_receive_frame faild";
        av_frame_free(&videoFrame);
        return;
    }

    QImage newFrame = this->convertToQImage(videoFrame, video_sws_ctx);
    QPixmap pixmap = QPixmap::fromImage(newFrame);
    if (!pixmap.isNull()) {
        emit this->videoImageChanged(pixmap);
    }
    av_frame_free(&videoFrame);
}

void DecodeVideo::decodeLoop() {

    double videoPts = 0.0;
    qDebug() << "===DecodeVideo::decodeLoop===";

    int64_t startTime = av_gettime();

    while (this->is_playing) {
        if (this->packet_queue->count() == 0) {
            av_usleep(1000);
            continue;
        }

        AVPacket *pkt = this->packet_queue->dequeue();
        if (pkt && pkt->stream_index == this->video_stream_index) {

            // 计算当前时间
            videoPts = av_q2d(fmt_ctx->streams[video_stream_index]->time_base) * pkt->pts;

            int64_t currentTime = (videoPts - skip_duration) * 1000 * 1000 + startTime;
            duration = videoPts;

            int64_t diff = currentTime - av_gettime();

            if (diff > 50 * 1000) {
                av_usleep(diff / 2);
            } else if (diff < -50 * 1000) {
                av_packet_unref(pkt);
                av_packet_free(&pkt);
                qDebug() << "===continue===";
                continue;
            }

            this->decodeVideoFrame(pkt);
            emit currentTimeChanged(videoPts);
        }
        if (pkt) {
//            av_packet_unref(pkt);
            av_packet_free(&pkt);
        }
    }

}

void DecodeVideo::closeFile() {

    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
        fmt_ctx = nullptr;
    }
    if (video_dec_ctx) {
        avcodec_free_context(&video_dec_ctx);
        video_dec_ctx = nullptr;
    }

    if (video_sws_ctx) {
        sws_freeContext(video_sws_ctx);
        video_sws_ctx = nullptr;
    }

}


bool DecodeVideo::startOrPause(bool pause) {

    start_or_pause_mutex.lock();
    this->is_playing = pause;
    if (this->is_playing) {
        videoDecoderThread = new VideoDecoderThread(this);
        QThreadPool::globalInstance()->start(videoDecoderThread);
    } else {
        this->skip_duration = duration;
        videoDecoderThread = nullptr;
    }
    start_or_pause_mutex.unlock();
    return this->is_playing;
}

DecodeVideo::~DecodeVideo() {
    this->closeFile();
}

void DecodeVideo::addPacket(AVPacket *pkt) {
//    my_mutex.lock();
//    AVPacket *pkt1 = av_packet_clone(pkt);;
//    if (!pkt1) {
//        return;
//    }
//
//    this->packet_queue->enqueue(pkt1);
//    my_mutex.unlock();

    AVPacket *pkt1 = av_packet_alloc();
    if (!pkt1) {
        av_packet_unref(pkt1);
    }
    av_packet_move_ref(pkt1, pkt);

    my_mutex.lock();
    this->packet_queue->enqueue(pkt1);
    my_mutex.unlock();
}

void DecodeVideo::resetPacketQueue() {
    my_mutex.lock();
    this->packet_queue->clear();
    my_mutex.unlock();
}

