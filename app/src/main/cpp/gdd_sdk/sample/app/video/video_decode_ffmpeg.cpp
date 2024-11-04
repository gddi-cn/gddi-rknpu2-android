#include "video_decode_ffmpeg.h"

#include <stdio.h>
#include <string>
#include <thread>
#include <iostream>
#include <queue>

#include "common/logger.h"
#include "opencv2/opencv.hpp"

#ifdef WITH_RK
#include "rk/mpp_decoder.h"
#include "common/rk_rga_mem.h"
#endif
namespace gddeploy {
class DecoderPrivate {
 public:
    DecoderPrivate();
    ~DecoderPrivate();

    int open_stream(std::string url_path, DecodeCallback callback);

private:
    std::queue<AVPacket*> packet_queue; // 添加一个队列来存储待解码的数据包
    const int MAX_QUEUE_SIZE = 30; // 定义队列的最大长度
    bool discard_non_key_frames = false; // 标志是否丢弃非关键帧

    int demux_stream(const std::string &stream_url,
                  const std::function<bool(const AVCodecParameters *)> &on_open,
                  const std::function<bool(const AVPacket *)> &on_packet);

    bool decode_packet(const AVPacket *packet, DecodeCallback callback);
    bool open_decoder(const AVCodecParameters *codecpar);

    AVFormatContext *fmt_ctx = nullptr;
    AVCodecParameters *codecpar = nullptr;
    AVPacket *packet = nullptr;

    AVCodec *decoder = nullptr;
    AVCodecContext *codec_ctx = nullptr;
    AVBSFContext* bsf_ctx = nullptr;
#ifdef WITH_RK
    std::shared_ptr<av_wrapper::MppDecoder> mpp_decoder_;
#endif
    int frame_num = 0;
};
}

using namespace gddeploy;
DecoderPrivate::DecoderPrivate() 
{
    fmt_ctx = avformat_alloc_context();
    codecpar = avcodec_parameters_alloc();
    packet = av_packet_alloc();
#ifdef WITH_RK
    mpp_decoder_ = std::make_shared<av_wrapper::MppDecoder>();
#endif
}

DecoderPrivate::~DecoderPrivate() {
  avcodec_free_context(&codec_ctx);
  avformat_close_input(&fmt_ctx);
//   av_buffer_unref(&hw_device_ctx_);
}

AVPixelFormat hw_pix_fmt{AV_PIX_FMT_NONE};

int DecoderPrivate::open_stream(std::string url_path, DecodeCallback callback)
{
    return demux_stream(url_path, [this](const AVCodecParameters *codecpar) { return this->open_decoder(codecpar); },
            [this, callback](const AVPacket *packet) { return this->decode_packet(packet, callback); });
}

// static enum AVPixelFormat hw_pix_fmt;
enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {
    auto _this = reinterpret_cast<DecoderPrivate *>(ctx->opaque);
    if (_this) {
        for (const enum AVPixelFormat *p = pix_fmts; *p != -1; p++) {
            if (*p == hw_pix_fmt) return *p;
        }
        GDDEPLOY_ERROR("[gddeploy] [ffmpeg] Failed to get HW surface format.");
    }
    // const enum AVPixelFormat *format;
    // for (format = pix_fmts; *format != -1; format++) {
    //     if (*format == hw_pix_fmt) return *format;
    // }
    return AV_PIX_FMT_NONE;
}


int DecoderPrivate::demux_stream(const std::string &stream_url,
                  const std::function<bool(const AVCodecParameters *)> &on_open,
                  const std::function<bool(const AVPacket *)> &on_packet) {
    AVDictionary *opts = nullptr;

    if (avformat_network_init()) {
        GDDEPLOY_ERROR("[gddeploy] [ffmpeg] avformat_network_init fail ");
        return -1;
    }

    // if (!strncmp(stream_url.c_str(), "rtsp", 4) || !strncmp(stream_url.c_str(), "rtmp", 4)) {
    //     av_log(fmt_ctx, AV_LOG_INFO, "decode rtsp/rtmp stream\n");
    //     av_dict_set(&opts, "buffer_size",    "1024000",  0);
    //     av_dict_set(&opts, "max_delay",      "500000",   0);
    //     av_dict_set(&opts, "stimeout",       "20000000", 0);
    //     av_dict_set(&opts, "rtsp_transport", "tcp",      0);
    // } else {
    //     av_log(fmt_ctx, AV_LOG_INFO, "decode local file stream\n");
    //     av_dict_set(&opts, "stimeout", "20000000", 0);
    //     av_dict_set(&opts, "vsync", "0",           0);
    // }
    av_dict_set(&opts, "buffer_size", "1024000", 0);
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    int ret = 0;
    if ((ret = avformat_open_input(&fmt_ctx, stream_url.c_str(), nullptr, &opts)) != 0) {
        GDDEPLOY_ERROR("[gddeploy] [ffmpeg] couldn't open input stream: {}, ret:{}", stream_url, ret);
        return -1;
    }

    av_dict_free(&opts);

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        GDDEPLOY_ERROR("[gddeploy] [ffmpeg] couldn't find stream information. ", stream_url);
        return -1;
    }

    int real_video_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (real_video_index < 0) {
        GDDEPLOY_ERROR("[gddeploy] [ffmpeg] didn't find a video stream.", stream_url);
        return -1;
    }

    avcodec_parameters_copy(codecpar, fmt_ctx->streams[real_video_index]->codecpar);
    if (!on_open(codecpar)) { return -1; }
#if defined(WITH_RK)
    // 确认编解码器类型后初始化比特流过滤器
    if(codecpar->codec_id == AV_CODEC_ID_H264) {
        const AVBitStreamFilter* bsfilter = av_bsf_get_by_name("h264_mp4toannexb");
        av_bsf_alloc(bsfilter, &bsf_ctx);
        avcodec_parameters_copy(bsf_ctx->par_in, codecpar);
        av_bsf_init(bsf_ctx);
    }
#endif
    int isNeedKeyFrame = 1;
    if (on_packet) {
        while (true) {
            // usleep(30*1000);
            int read_ret_val = av_read_frame(fmt_ctx, packet);
            if (read_ret_val >= 0) {
#if defined(WITH_TS)
                if (packet->stream_index == real_video_index) { 
                    do{
                        if (isNeedKeyFrame) {
                            printf("Packet size: %d\n", packet->size);
                            if (packet->flags & AV_PKT_FLAG_KEY){
                                // printf("get key frame\n");
                                isNeedKeyFrame = 0;
                            }else{
                                // printf("not key frame\n");
                                break;
                            }
                        }
                        on_packet(packet);   
                    }while(0);
                }
#elif defined(WITH_RK)
                if (packet->stream_index == real_video_index) {
                    // 对于 H.264 视频流的包，应用比特流过滤器
                    if (codecpar->codec_id == AV_CODEC_ID_H264 && bsf_ctx) {
                        av_bsf_send_packet(bsf_ctx, packet);
                        while (av_bsf_receive_packet(bsf_ctx, packet) == 0) {
                            // usleep(40*1000);
                            on_packet(packet);
                        }
                    } else {
                        on_packet(packet);
                    }
                }
#else 
                if (packet->stream_index == real_video_index) { on_packet(packet); }
#endif
                av_packet_unref(packet);
            } else {
                printf("Exit Packet size: %d\n", packet->size);
                return -1;
            }
        }
    }
#if defined(WITH_RK)
    // 清理资源
    if (bsf_ctx) {
        av_bsf_free(&bsf_ctx);
    }
#endif
    return 0;
}

bool DecoderPrivate::open_decoder(const AVCodecParameters *codecpar) {
    AVDictionary *opts = nullptr;
#ifdef WITH_BM1684
    if (codecpar->codec_id == AV_CODEC_ID_MJPEG
        && codecpar->profile == FF_PROFILE_MJPEG_HUFFMAN_BASELINE_DCT) {
        decoder = avcodec_find_decoder_by_name("jpeg_bm");
    } else {
        decoder = avcodec_find_decoder(codecpar->codec_id);
    }
#elif defined(WITH_MLU220) || defined(WITH_MLU270) || defined(WITH_MLU370)
    switch (codecpar->codec_id) {
        case AV_CODEC_ID_H264: decoder = avcodec_find_decoder_by_name("h264_mludec"); break;
        case AV_CODEC_ID_HEVC: decoder = avcodec_find_decoder_by_name("hevc_mludec"); break;
        case AV_CODEC_ID_VP8: decoder = avcodec_find_decoder_by_name("vp8_mludec"); break;
        case AV_CODEC_ID_VP9: decoder = avcodec_find_decoder_by_name("vp9_mludec"); break;
        case AV_CODEC_ID_MJPEG: decoder = avcodec_find_decoder_by_name("mjpeg_mludec"); break;
        default: decoder = avcodec_find_decoder(codecpar->codec_id); break;
    }
#elif defined(WITH_JETSON)
    switch (codecpar->codec_id) {
        case AV_CODEC_ID_H264: decoder = avcodec_find_decoder_by_name("h264_nvv4l2dec"); break;
        case AV_CODEC_ID_HEVC: decoder = avcodec_find_decoder_by_name("hevc_nvv4l2dec"); break;
        case AV_CODEC_ID_VP8: decoder = avcodec_find_decoder_by_name("vp8_nvv4l2dec"); break;
        case AV_CODEC_ID_VP9: decoder = avcodec_find_decoder_by_name("vp9_nvv4l2dec"); break;
        case AV_CODEC_ID_MPEG2TS: decoder = avcodec_find_decoder_by_name("mpeg2_nvv4l2dec"); break;
        case AV_CODEC_ID_MPEG4: decoder = avcodec_find_decoder_by_name("mpeg4_nvv4l2dec"); break;
        default: decoder = avcodec_find_decoder(codecpar->codec_id); break;
    }
#elif defined(WITH_NVIDIA)
    decoder = avcodec_find_decoder_by_name("h264_cuvid");
#elif defined(WITH_RV1126)
    switch (codecpar->codec_id) {
        case AV_CODEC_ID_H264: decoder = avcodec_find_decoder_by_name("h264_rkmpp"); break;
        case AV_CODEC_ID_HEVC: decoder = avcodec_find_decoder_by_name("hevc_rkmpp"); break;
        case AV_CODEC_ID_VP8: decoder = avcodec_find_decoder_by_name("vp8_rkmpp"); break;
        case AV_CODEC_ID_VP9: decoder = avcodec_find_decoder_by_name("vp9_rkmpp"); break;
        default: decoder = avcodec_find_decoder(codecpar->codec_id); break;
    }
#elif defined(WITH_TS)
    // decoder = (AVCodec*)avcodec_find_decoder(AV_CODEC_ID_H264);
    switch (codecpar->codec_id) {
        case AV_CODEC_ID_H264: decoder = (AVCodec*)avcodec_find_decoder_by_name("h264_tsmpp"); 
            printf("!!!!!!!!!!!!!!!!!decoder h264_tsmpp\n");
            break;
        case AV_CODEC_ID_HEVC: decoder = (AVCodec*)avcodec_find_decoder_by_name("hevc_tsmpp"); 
            printf("!!!!!!!!!!!!!!!!!decoder h265_tsmpp\n");
            break;
    }
	av_dict_set_int(&opts, "width", codecpar->width, 0);
	av_dict_set_int(&opts, "height", codecpar->height, 0);
	av_dict_set_int(&opts, "framebuf_cnt", 25, 0);
	av_dict_set_int(&opts, "refframebuf_num", 25, 0);

#elif defined(WITH_INTEL)
    // switch (codecpar->codec_id) {
    //     case AV_CODEC_ID_H264: decoder = (AVCodec*)avcodec_find_decoder_by_name("h264_vaapi"); break;
    //     case AV_CODEC_ID_HEVC: decoder = (AVCodec*)avcodec_find_decoder_by_name("hevc_vaapi"); break;
    //     case AV_CODEC_ID_VP8: decoder = (AVCodec*)avcodec_find_decoder_by_name("vp8_vaapi"); break;
    //     case AV_CODEC_ID_VP9: decoder = (AVCodec*)avcodec_find_decoder_by_name("vp9_vaapi"); break;
    //     case AV_CODEC_ID_MPEG2TS: decoder = avcodec_find_decoder_by_name("mpeg2_vaapi"); break;
    //     case AV_CODEC_ID_MPEG4: decoder = avcodec_find_decoder_by_name("mjpeg_vaapi"); break;
    //     default: decoder = avcodec_find_decoder(codecpar->codec_id); break;
    // }
    decoder = avcodec_find_decoder(codecpar->codec_id);
#else
    decoder = avcodec_find_decoder(codecpar->codec_id);
#endif

    if (decoder == nullptr) {
        GDDEPLOY_ERROR("[gddeploy] [ffmpeg] Failed to find decoder: {}", avcodec_get_name(codecpar->codec_id));
        return false;
    }
    codec_ctx = avcodec_alloc_context3(decoder);
        
    // codec_ctx->get_format = get_hw_format;

    avcodec_parameters_to_context(codec_ctx, codecpar);

    AVHWDeviceType dev_type = AV_HWDEVICE_TYPE_NONE;
#if defined(WITH_INTEL)
    dev_type = AV_HWDEVICE_TYPE_VAAPI;// = av_hwdevice_find_type_by_name("h264");
    // for (int i = 0;; i++) {
    //     const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
    //     if (!config) {
    //         std::cout << "[gddeploy Samples] [FFmpegMluDecodeImpl] Decoder " << decoder->name << " doesn't support device type "
    //                     << av_hwdevice_get_type_name(dev_type) << std::endl;
    //         return false;
    //     }
    //     if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == dev_type) {
    //         // std::cout << "hw pix fmt: " << config->pix_fmt << std::endl;
    //         hw_pix_fmt = config->pix_fmt;
    //         break;
    //     }
    // }
       AVBufferRef *hw_device_ctx = nullptr;
    if (av_hwdevice_ctx_create(&hw_device_ctx, dev_type, NULL, NULL, 0)) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return false;
    }
    codec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
#elif defined(WITH_NVIDIA)
    AVBufferRef *hw_device_ctx = nullptr;
    AVHWFramesContext *frames_ctx;
    // 初始化硬件设备类型为CUDA
    if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, NULL, NULL, 0) != 0) {
        fprintf(stderr, "Failed to create CUDA HW device.\n");
        return false;
    }

    codec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    // 初始化硬件帧上下文
    codec_ctx->hw_frames_ctx = av_hwframe_ctx_alloc(codec_ctx->hw_device_ctx);
    if (!codec_ctx->hw_frames_ctx) {
        fprintf(stderr, "Failed to create CUDA HW frames context.\n");
        return false;
    }

    frames_ctx = (AVHWFramesContext *)(codec_ctx->hw_frames_ctx->data);
    frames_ctx->format = AV_PIX_FMT_CUDA;  // 硬件像素格式
    frames_ctx->sw_format = AV_PIX_FMT_NV12;  // 软件像素格式，即你想要转换成的格式
    frames_ctx->width = codec_ctx->width;
    frames_ctx->height = codec_ctx->height;

    if (av_hwframe_ctx_init(codec_ctx->hw_frames_ctx) < 0) {
        fprintf(stderr, "Failed to initialize CUDA HW frames context.\n");
        return false;
    }

#endif



#if defined(WITH_BM1684)
    // av_dict_set_int(&opts, "output_format", 101, 0);
    // av_dict_set_int(&opts, "extra_frame_buffer_num", 9, 0);

    // av_dict_set_int(&opts, "enable_cache", 1, 0);
    av_dict_set_int(&opts, "chroma_interleave", 1, 0);
    av_dict_set_int(&opts, "bs_buffer_size", 20480, 0);
    // av_dict_set(&opts, "cbcr_interleave", "0", 0);
#elif defined(WITH_MLU220)
    // av_dict_set(&opts, "device_id", 0, 0);
    // av_dict_set(&opts, "output_pixfmt", "mlu", 0);
#endif
    if (avcodec_open2(codec_ctx, decoder, &opts) < 0) { return false; }

    av_dict_free(&opts);

#if WITH_RK
    return mpp_decoder_->open_decoder(MppCodingType::MPP_VIDEO_CodingAVC,codec_ctx->width,codec_ctx->height);
#else
    return true;
#endif
}


bool DecoderPrivate::decode_packet(const AVPacket *packet, DecodeCallback callback) {
    int frame_idx = 0;
#if WITH_RK
        av_wrapper::CodecCallback codec_callback = [callback, frame_idx](const int64_t image_id, std::shared_ptr<RkRgaMem> rk_rga_mem) mutable {
            callback(frame_idx++, rk_rga_mem);
            return true;
        };

        return mpp_decoder_->decode(packet->data, packet->size, codec_callback);
#else
    int ret = avcodec_send_packet(codec_ctx, packet);
    if (ret < 0) {
        GDDEPLOY_ERROR("[gddeploy] [ffmpeg] Error submitting a packet for decoding");
        return false;
    }
    
    while (ret >= 0) {
        // AVFrame *frame = av_frame_alloc();
        auto avframe = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *ptr) { 
            av_frame_free(&ptr); 
            });

        ret = avcodec_receive_frame(codec_ctx, avframe.get());
        if (ret < 0) {
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) 
              return true;

            GDDEPLOY_ERROR("[gddeploy] [ffmpeg] Error during decoding");
            return ret;
        }

        if (codec_ctx->codec->type == AVMEDIA_TYPE_VIDEO) { 
            frame_num++;

            // if (frame_num % 10 == 0 || frame_num%10 == 2 || frame_num%10 == 4 || frame_num%10 == 6) {
            //     break;
            // } 
            if (avframe->format == AV_PIX_FMT_QSV || avframe->format == AV_PIX_FMT_VAAPI) {
                std::shared_ptr<AVFrame> hw_frame = std::shared_ptr<AVFrame>(av_frame_alloc(), [](AVFrame *ptr) { 
                    av_frame_free(&ptr); 
                });

                if (av_hwframe_transfer_data(hw_frame.get(), avframe.get(), 0) < 0) {
                    throw std::runtime_error("Error transferring the data to system memory");
                }
                av_frame_copy_props(hw_frame.get(), avframe.get());

                callback(frame_idx++, hw_frame); 
            } else {
                callback(frame_idx++, avframe); 
            }
        }
    }

    return true;
#endif
}

namespace gddeploy{
Decoder::Decoder() 
{ 
    decoder_priv_ = std::make_shared<DecoderPrivate>(); 
}

int Decoder::open_stream(const std::string &stream_url, DecodeCallback decode_cb) {
    return decoder_priv_->open_stream(stream_url, decode_cb);
}
}