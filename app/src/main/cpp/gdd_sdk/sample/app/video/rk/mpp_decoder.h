#pragma once

#if defined(WITH_RK)

#include <rk_mpi.h>
#include <rk_venc_cfg.h>

#include <cstdint>
#include <functional>
#include <vector>
#include "rga.h"
#include "common/rk_rga_mem.h"

namespace av_wrapper {

enum class CodingType { kH264, kHEVC };

using CodecCallback =
    std::function<void(const int64_t image_id,std::shared_ptr<RkRgaMem> rk_rga_mem)>;

class MppDecoder {
public:
    MppDecoder();
    ~MppDecoder();

    bool open_decoder(const MppCodingType type, const int width, const int height);
    bool decode(const uint8_t *data, const uint32_t size, const CodecCallback &callback);
    void close_decoder();

private:
    MppCtx ctx = nullptr;
    MppApi *mpi = nullptr;
    

    char *pkt_buf = nullptr;
    MppPacket packet = nullptr;

    MppBuffer frm_buf_ = nullptr;

    MppBufferGroup frm_grp_ = nullptr;

    uint32_t image_id = 0;

    uint32_t width_;
    uint32_t height_;
    uint32_t hor_stride_;
    uint32_t ver_stride_;
    uint32_t buf_size_;
    MppFrameFormat format_;
};

}// namespace av_wrapper

#endif