#pragma once
#if WITH_RK
#include <getopt.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/eventfd.h>

#include <sched.h>
#include <pthread.h>

#include <stdint.h>
#include <math.h>
#include <memory.h>
#include <sys/time.h>

#include "im2d.hpp"

#include "RgaUtils.h"
#include <mpp_frame.h>
#include <mpp_packet.h>

#define DMA_HEAP_UNCACHE_PATH           "/dev/dma_heap/system-uncached"
#define DMA_HEAP_PATH                   "/dev/dma_heap/system"
#define DMA_HEAP_DMA32_UNCACHE_PATCH    "/dev/dma_heap/system-uncached-dma32"
#define DMA_HEAP_DMA32_PATCH            "/dev/dma_heap/system-dma32"
#define CMA_HEAP_UNCACHE_PATH           "/dev/dma_heap/cma-uncached"
#define RV1106_CMA_HEAP_PATH	        "/dev/rk_dma_heap/rk-dma-heap-cma"

typedef unsigned long long __u64;
typedef  unsigned int __u32;

struct dma_heap_allocation_data {
	__u64 len;
	__u32 fd;
	__u32 fd_flags;
	__u64 heap_flags;
};

#define DMA_HEAP_IOC_MAGIC		'H'
#define DMA_HEAP_IOCTL_ALLOC	_IOWR(DMA_HEAP_IOC_MAGIC, 0x0,\
				      struct dma_heap_allocation_data)

#define DMA_BUF_SYNC_READ      (1 << 0)
#define DMA_BUF_SYNC_WRITE     (2 << 0)
#define DMA_BUF_SYNC_RW        (DMA_BUF_SYNC_READ | DMA_BUF_SYNC_WRITE)
#define DMA_BUF_SYNC_START     (0 << 2)
#define DMA_BUF_SYNC_END       (1 << 2)

struct dma_buf_sync {
	__u64 flags;
};

#define DMA_BUF_BASE		'b'
#define DMA_BUF_IOCTL_SYNC	_IOW(DMA_BUF_BASE, 0, struct dma_buf_sync)

#define CMA_HEAP_SIZE	1024 * 1024

inline int dma_sync_device_to_cpu(int fd) {
    struct dma_buf_sync sync = {0};

    sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;
    return ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
}

inline int dma_sync_cpu_to_device(int fd) {
    struct dma_buf_sync sync = {0};

    sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;
    return ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
}

inline int dma_buf_alloc(const char *path, size_t size, int *fd, void **va) {
    int ret;
    int prot;
    void *mmap_va;
    int dma_heap_fd = -1;
    struct dma_heap_allocation_data buf_data;

    /* open dma_heap fd */
    dma_heap_fd = open(path, O_RDWR);
    if (dma_heap_fd < 0) {
        printf("open %s fail!\n", path);
        return dma_heap_fd;
    }

    /* alloc buffer */
    memset(&buf_data, 0x0, sizeof(struct dma_heap_allocation_data));

    buf_data.len = size;
    buf_data.fd_flags = O_CLOEXEC | O_RDWR;
    ret = ioctl(dma_heap_fd, DMA_HEAP_IOCTL_ALLOC, &buf_data);
    if (ret < 0) {
        printf("RK_DMA_HEAP_ALLOC_BUFFER failed\n");
        return ret;
    }

    /* mmap va */
    if (fcntl(buf_data.fd, F_GETFL) & O_RDWR)
        prot = PROT_READ | PROT_WRITE;
    else
        prot = PROT_READ;

    /* mmap contiguors buffer to user */
    mmap_va = (void *)mmap(NULL, buf_data.len, prot, MAP_SHARED, buf_data.fd, 0);
    if (mmap_va == MAP_FAILED) {
        printf("mmap failed: %s\n", strerror(errno));
        return -errno;
    }

    *va = mmap_va;
    *fd = buf_data.fd;

    close(dma_heap_fd);

    return 0;
}

inline void dma_buf_free(size_t size, int *fd, void *va) {
    int len;

    len =  size;
    munmap(va, len);

    close(*fd);
    *fd = -1;
}

class RkRgaMemInfo
{
public:
    int dma_fd_;
    rga_buffer_handle_t rga_handle_;
    size_t size_;
    char* buf_;
    rga_buffer_t rga_buffer_;
    RkRgaMemInfo(){
        dma_fd_ = -1;
        rga_handle_ = -1;
        size_ = 0;
        buf_ = nullptr;
        is_alloc_ = false;
    };
    RkRgaMemInfo(size_t size,char* &buf)
    {
        int ret = 0;
        ret = dma_buf_alloc(DMA_HEAP_DMA32_UNCACHE_PATCH, size, &dma_fd_, (void **)&buf);
        // printf("dma_buf_alloc %d\n", block_size_ * surf->batch_size);
        if (ret < 0) {
            printf("alloc src dma32_heap buffer failed!\n");
            assert(0);
        }
        rga_handle_ = importbuffer_fd(dma_fd_, size);    
        if (rga_handle_ == 0) {
            printf("import dma_fd error!\n");
            dma_buf_free(size, &dma_fd_, buf);
            assert(0);
        }
        size_ = size;
        buf_ = buf;
        is_alloc_ = true;
    };
    ~RkRgaMemInfo()
    {
        if(is_alloc_ == true)
        {
            if(buf_ != nullptr){
                dma_buf_free(size_, &dma_fd_, buf_);
                buf_  = nullptr;
            }
            if(rga_handle_ > 0)
            {
                releasebuffer_handle(rga_handle_);
                rga_handle_ = -1;
            }
        }
        
    };
private:
    bool is_alloc_;
};

inline RgaSURF_FORMAT mppfmt_to_rgafmt(const MppFrameFormat format) {
    switch (format) {
        case MPP_FMT_YUV420SP: return RgaSURF_FORMAT::RK_FORMAT_YCbCr_420_SP;
        case MPP_FMT_YUV420P: return RgaSURF_FORMAT::RK_FORMAT_YCbCr_420_P;
        case MPP_FMT_YUV422P: return RgaSURF_FORMAT::RK_FORMAT_YCbCr_422_P;
        case MPP_FMT_YUV422SP: return RgaSURF_FORMAT::RK_FORMAT_YCbCr_422_SP;
        default: return RgaSURF_FORMAT::RK_FORMAT_UNKNOWN;
    }
};

class RkRgaMem
{
private:
    MppFrame mpp_frame_;
    static int rk_mem_count;
    
public:
    uint32_t width;
    uint32_t height;
    uint32_t hor_stride;
    uint32_t ver_stride;
    RgaSURF_FORMAT format;
    MppBuffer frm_buf_;

    std::shared_ptr<RkRgaMemInfo> mem_info;

    // RkRgaMem(){
    //     mpp_frame_ = nullptr;
    // };
    RkRgaMem(void* mpp_frame, MppBuffer frm_buf){
        frm_buf_ = frm_buf;
        mpp_frame_ = (MppFrame)mpp_frame;
        width = mpp_frame_get_width(mpp_frame_);
        height = mpp_frame_get_height(mpp_frame_);
        hor_stride = mpp_frame_get_hor_stride(mpp_frame_);
        ver_stride = mpp_frame_get_ver_stride(mpp_frame_);
        format = mppfmt_to_rgafmt(mpp_frame_get_fmt(mpp_frame_));
        mem_info = std::make_shared<RkRgaMemInfo>();
        
        mem_info->dma_fd_ = mpp_buffer_get_fd(mpp_frame_get_buffer(mpp_frame_));
        mem_info->rga_handle_ = importbuffer_fd(mem_info->dma_fd_, hor_stride * ver_stride * 3 / 2);
        mem_info->size_ = hor_stride * ver_stride * 3 / 2;
        mem_info->buf_ = (char*)mpp_buffer_get_ptr(mpp_frame_get_buffer(mpp_frame_));
        rk_mem_count++;
        // if(rk_mem_count > 10)
        // {
        //     usleep(1000*500);
        // }
        //printf all info in RkRgaMem
        // printf("zzy debug create RkRgaMem this 0x%x mem_info addr 0x%x mpp_frame_ 0x%x width %d height %d hor_stride %d ver_stride %d format %d dma_fd %d rga_handle %d size %d buf 0x%x frm_buf_ 0x%x count %d\n",\
         this,mem_info.get(),mpp_frame_,width,height,hor_stride,ver_stride,format,mem_info->dma_fd_,mem_info->rga_handle_, mem_info->size_,mem_info->buf_,frm_buf_,rk_mem_count);
    };
    ~RkRgaMem(){
        if(mpp_frame_ != nullptr)
        {
            rk_mem_count--;
            // printf("zzy debug release RkRgaMem mpp_frame_ 0x%x count %d\n",mpp_frame_,rk_mem_count);
            mpp_frame_deinit(&mpp_frame_);
            mpp_frame_ = nullptr;
        }
        if (frm_buf_) {
            // printf("zzy debug release RkRgaMem this 0x%x frm_buf_ 0x%x count %d\n", this,frm_buf_ ,rk_mem_count);
            mpp_buffer_put(frm_buf_);
            frm_buf_ = nullptr;
        }
        releasebuffer_handle(mem_info->rga_handle_);
    };
    static int get_mem_count() {
        return rk_mem_count;
    }
};


#endif