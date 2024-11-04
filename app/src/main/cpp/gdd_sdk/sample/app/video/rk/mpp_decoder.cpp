#if defined(WITH_RK)
#include <cstdint>
#include <mpp_frame.h>
#include <mpp_packet.h>
#include <sys/types.h>
#include <unistd.h>
#include "common/logger.h"
#include "opencv2/opencv.hpp"
#include "mpp_decoder.h"

#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
int RkRgaMem::rk_mem_count = 0;
namespace av_wrapper {

MppDecoder::MppDecoder() {}

MppDecoder::~MppDecoder() { close_decoder(); }

bool MppDecoder::open_decoder(const MppCodingType coding_type, const int width, const int height) {
    width_ = width;
    height_ = height;
    close_decoder();

    // 初始化MPP
    if (mpp_create(&ctx, &mpi) != MPP_OK) {
        GDDEPLOY_ERROR("mpp_create failed");
        return false;
    }

    // 配置解码器
    uint32_t need_split = -1;
    if (mpi->control(ctx, MPP_DEC_SET_PARSER_SPLIT_MODE, (MppParam *)&need_split) != MPP_OK) {
        GDDEPLOY_ERROR("mpi->control failed");
        return false;
    }

    if (mpp_init(ctx, MPP_CTX_DEC, coding_type) != MPP_OK) {
        GDDEPLOY_ERROR("mpp_init failed");
        return false;
    }

    if (mpp_buffer_group_get_internal(&frm_grp_, MppBufferType(MPP_BUFFER_TYPE_DMA_HEAP | MPP_BUFFER_FLAGS_DMA32)) != MPP_OK)
    // if (mpp_buffer_group_get_internal(&frm_grp_, MppBufferType(MPP_BUFFER_TYPE_ION)) != MPP_OK)
    {
        GDDEPLOY_ERROR("mpp_buffer_group_get_internal failed");
        return false;
    }

    if (mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, frm_grp_) != MPP_OK) {
        GDDEPLOY_ERROR("{} set buffer group failed", ctx);
        return false;
    }

    

    return true;
}

bool MppDecoder::decode(const uint8_t *data, const uint32_t size, const CodecCallback &callback)
{
    MppPacket packet = nullptr;
    mpp_packet_init(&packet, const_cast<uint8_t *>(data), size);
    if (!packet) {
        GDDEPLOY_ERROR("Failed to initialize packet");
        return false;
    }

    // 使用智能指针自动释放 packet 资源
    auto packet_guard = std::shared_ptr<void>(nullptr, [&packet](...){
        mpp_packet_deinit(&packet);
    });

    bool ret = true;
    uint32_t pkt_done = 0;
    do {
        if (mpi->decode_put_packet(ctx, packet) == MPP_OK) {
            pkt_done = 1;
        }

        MppFrame frame = nullptr;
        if (mpp_frame_init(&frame) != MPP_OK) {
            GDDEPLOY_ERROR("{} decoder failed to init frame", ctx);
            ret = false;
            break;
        }

        // 使用智能指针自动释放 frame 资源
        auto frame_guard = std::shared_ptr<void>(nullptr, [&frame](...){
            if (frame) {
                mpp_frame_deinit(&frame);
            }
        });
        printf("zzy debug width_ %d ALIGN(width_, 16) %d height_ %d ALIGN(height_, 16) %d\n",width_,ALIGN(width_, 16),height_,ALIGN(height_, 16));
        if (mpp_buffer_get(frm_grp_, &frm_buf_, ALIGN(width_, 16) * ALIGN(height_, 16) * 3 / 2) != MPP_OK) {
            GDDEPLOY_ERROR("mpp_buffer_get failed");
            ret = false;
            break;
        }else
        {
            GDDEPLOY_INFO("mpp_buffer_get success");
        }

        // 使用智能指针自动释放 frm_buf_ 资源
        auto buffer_guard = std::shared_ptr<void>(nullptr, [this](...){
            if (frm_buf_) {
                mpp_buffer_put(frm_buf_);
                frm_buf_ = nullptr;
            }
        });

        mpp_frame_set_buffer(frame, frm_buf_);

        if (mpi->decode_get_frame(ctx, &frame) != MPP_OK || !frame) {
            usleep(2000);
            ret = false;
            break;
        }

        if (mpp_frame_get_info_change(frame)) {
            if (mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, nullptr)) {
                GDDEPLOY_ERROR("{} decoder info change ready failed", ctx);
            }
            continue;
        }

        ++image_id;
        if(mpp_frame_get_width(frame) != 0 && mpp_frame_get_height(frame) != 0 \
            && mpp_frame_get_hor_stride(frame) != 0 && mpp_frame_get_ver_stride(frame) != 0 && \
            mpp_buffer_get_ptr(mpp_frame_get_buffer(frame)) != nullptr && \
            callback != nullptr)
        {
            auto rk_rga_mem = std::make_shared<RkRgaMem>(frame, frm_buf_);
            do{
                if(RkRgaMem::get_mem_count() > 10)
                {
                    GDDEPLOY_INFO("RkRgaMem::get_mem_count({}) > 10, sleep 20ms", RkRgaMem::get_mem_count());
                    usleep(20000);
                }
                else
                {
                    break;
                }
            }while(true);
            callback(image_id, rk_rga_mem);
            // 注意：如果在回调函数中需要保留 rk_rga_mem，确保它的生命周期被正确管理
        }
    } while (!pkt_done);

    return ret;
}


void MppDecoder::close_decoder() {
    if (mpi) { mpi->reset(ctx); }

    if (ctx) { mpp_destroy(ctx); }

    if (frm_grp_) {
        mpp_buffer_group_put(frm_grp_);
        frm_grp_ = nullptr;
    }
}

}// namespace av_wrapper

#endif