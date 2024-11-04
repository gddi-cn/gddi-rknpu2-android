#include "app/runner_video.h"
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

#include "common/logger.h"
#include "core/mem/buf_surface_util.h"
#include "core/mem/buf_surface_impl.h"
#include "common/data_queue.h"
#include "api/infer_api.h"
#include "api/global_config.h"
#include "common/type_convert.h"
#include "app/result_handle.h"
#include "video/video_decode_ffmpeg.h"
#include <any>
extern "C" {
    #include <libswscale/swscale.h>
}
#ifdef WITH_RK
#include "common/rk_rga_mem.h"
#endif

#ifdef WITH_NVIDIA
#include "cuda_runtime.h"
#include "npp.h"
#endif
namespace gddeploy {
    static std::mutex g_mutex; 
class VideoRunnerPriv;
class VideoRunnerPriv{
public:
    VideoRunnerPriv()=default;
    ~VideoRunnerPriv(){
        is_exit_ = true;
        decoder_ = nullptr;
        condition_.notify_all();
        Join();
    }

    int Init(const std::string config, std::vector<std::string> model_paths, std::vector<std::string> license_paths);

    int OpenStream(std::string video_path, std::string save_path="", bool is_draw=false);  

    template<typename FrameType>
    void InitPoolSurf(const std::shared_ptr<FrameType> &avframe);

    int Join(){
        if (thread_decode_frame_.joinable()){
            thread_decode_frame_.join();
        }
        if (thread_infer_handle_.joinable()){
            thread_infer_handle_.join();
        }
        return 0;
    }
private:
    InferAPI infer_api_;
    std::vector<InferAPI> infer_api_v_;

    std::thread thread_decode_frame_;
    std::thread thread_infer_handle_;
    bool is_exit_ = false;
    std::mutex mutex_;                 //互斥锁
    std::condition_variable condition_;//条件变

    std::string model_path_;
    std::string save_path_;
    std::string input_url_;
    std::shared_ptr<Decoder> decoder_;
    bool is_init_pool_surf = false;
#ifdef WITH_RK
    SafeQueue<std::shared_ptr<RkRgaMem>> input_queue_;
#else
    SafeQueue<std::shared_ptr<AVFrame>> input_queue_;
#endif
    SafeQueue<BufSurfWrapperPtr> in_pool_surf_;
    SafeQueue<BufSurfWrapperPtr> out_pool_surf_;
    std::chrono::steady_clock::time_point benchmake_startime;
#ifdef WITH_RK
    std::shared_ptr<AVFrame> convertRkRgaMem2AVFrame(const std::shared_ptr<RkRgaMem> rkframe);
#endif
};

} // namespace gddeploy

using namespace gddeploy;

template<typename FrameType>
void VideoRunnerPriv::InitPoolSurf(const std::shared_ptr<FrameType> &avframe) 
{
    if(is_init_pool_surf == true)
        return;
    BufSurfaceCreateParams params;
    params.batch_size = 1;
#if WITH_BM1684
    params.mem_type = GDDEPLOY_BUF_MEM_BMNN;
#elif WITH_NVIDIA
    params.mem_type = GDDEPLOY_BUF_MEM_NVIDIA;
    params.alignment = calculateAlignSizeW(1920, 1, 2048);
    // params.mem_type = GDDEPLOY_BUF_MEM_SYSTEM;
#elif WITH_TS
    params.mem_type = GDDEPLOY_BUF_MEM_TS;
#elif WITH_RK
    params.mem_type = GDDEPLOY_BUF_MEM_RK_RGA;
    params.user_alloc = true;
    params.wstride = avframe->hor_stride;
    params.hstride = avframe->ver_stride;
    params.surface_list = reinterpret_cast<BufSurfaceParams *>(malloc(sizeof(BufSurfaceParams) * params.batch_size));
    memset(params.surface_list, 0, sizeof(BufSurfaceParams) * params.batch_size);
    for(uint32_t i = 0; i < params.batch_size; i++)
    {
        params.surface_list[i].data_ptr = avframe->mem_info->buf_;
        params.surface_list[i]._reserved[1] = avframe->mem_info.get();
    }            
#else 
    params.mem_type = GDDEPLOY_BUF_MEM_SYSTEM;
#endif
    params.device_id = 0;
    params.width = avframe->width;
    params.height = avframe->height;
#if WITH_RK
    params.color_format = GDDEPLOY_BUF_COLOR_FORMAT_NV12;
    params.size = avframe->hor_stride * avframe->ver_stride * 3 / 2;
#else
    params.color_format = GDDEPLOY_BUF_COLOR_FORMAT_NV12;
    params.size = avframe->linesize[0] * avframe->height * 3 / 2;
#endif
#if WITH_NVIDIA
    params.force_align_1 = 0;
#else
    params.force_align_1 = 1;
#endif
    params.bytes_per_pix = 1;
#if WITH_TS
    gddeploy::BufSurfWrapperPtr surf_ptr(new BufSurfaceWrapper(new BufSurface, true));
    BufSurface *surf = surf_ptr->GetBufSurface();
    memset(surf, 0, sizeof(BufSurface));
    CreateSurface(&params, surf);
    convertAVFrame2BufSurface1(avframe.get(), surf_ptr, true);
#elif WITH_RK
    BufSurface *surf;
    BufSurfaceCreate(&surf, &params);
    gddeploy::BufSurfWrapperPtr surf_ptr = std::make_shared<gddeploy::BufSurfaceWrapper>(surf, false);
#else
    gddeploy::BufSurfWrapperPtr surf_ptr = nullptr;
    const int max_pool_size = 5;

    for(int i = 0; i < max_pool_size; i++)
    {
        std::shared_ptr<BufSurfaceWrapper> tmp(new BufSurfaceWrapper(new BufSurface, true));
        BufSurface *surf = tmp->GetBufSurface();
        memset(surf, 0, sizeof(BufSurface));
        CreateSurface(&params, surf);
        this->in_pool_surf_.push(tmp);
    }
#endif
    is_init_pool_surf = true;
}

int VideoRunnerPriv::Init(const std::string config, std::vector<std::string> model_paths, std::vector<std::string> license_paths)
{
    gddeploy_init("");
    for (uint32_t i = 0; i < model_paths.size(); i++){
        std::string model_path = model_paths[i];
        std::string license_path = license_paths[i];

        InferAPI infer_api;
        if (0 != infer_api.Init(config, model_path, license_path, ENUM_API_SESSION_API))
        {
            return -1;
        }
        infer_api_v_.push_back(infer_api);
    }
    return 0;
}

static BufSurfaceColorFormat convertFormat(int format)
{
    if (format == AV_PIX_FMT_YUV420P)
        return GDDEPLOY_BUF_COLOR_FORMAT_YUV420;
    if( format == AV_PIX_FMT_YUVJ420P)
        return GDDEPLOY_BUF_COLOR_FORMAT_YUVJ420P;
    if (format == AV_PIX_FMT_NV12)
        return GDDEPLOY_BUF_COLOR_FORMAT_NV12;
    if (format == AV_PIX_FMT_NV21) 
        return GDDEPLOY_BUF_COLOR_FORMAT_NV21;
    if (format == AV_PIX_FMT_RGB24)
        return GDDEPLOY_BUF_COLOR_FORMAT_RGB;
    if (format == AV_PIX_FMT_CUDA)
        return GDDEPLOY_BUF_COLOR_FORMAT_NV12;
    return GDDEPLOY_BUF_COLOR_FORMAT_INVALID;
}

static int AVFrame_GetSize(AVFrame *in)
{
    int data_size = -1;
    if (in->format == AV_PIX_FMT_YUV420P || 
        in->format == AV_PIX_FMT_NV12 || 
        in->format == AV_PIX_FMT_NV21 || 
        in->format == AV_PIX_FMT_YUVJ420P ||
        in->format == AV_PIX_FMT_CUDA)
        data_size = in->height * in->linesize[0] * 3 / 2;

    if (in->format == AV_PIX_FMT_RGB24 || in->format == AV_PIX_FMT_BGR24)
        data_size = in->height * in->width * 3;

    return data_size;
}

static BufSurfaceColorFormat AVFrame_ChangeColor(AVFrame *in)
{
    if (in->format == AV_PIX_FMT_YUV420P)
        return GDDEPLOY_BUF_COLOR_FORMAT_YUV420;
    if (in->format == AV_PIX_FMT_RGB24)
        return GDDEPLOY_BUF_COLOR_FORMAT_RGB;
    
    return GDDEPLOY_BUF_COLOR_FORMAT_INVALID;
}
#if WITH_TS
#include "ts_comm_vdec.h"
#endif
static int convertAVFrame2BufSurface1(AVFrame *in, gddeploy::BufSurfWrapperPtr &surf, bool is_copy)
{
    BufSurface *dst_surf = surf->GetBufSurface();

    //TODO: 修改为根据设备类型调用不同的拷贝函数
    if (is_copy){
        BufSurface src_surf;
#if WITH_BM1684
        src_surf.mem_type = GDDEPLOY_BUF_MEM_BMNN;
#elif WITH_NVIDIA
        src_surf.mem_type = GDDEPLOY_BUF_MEM_NVIDIA;
        // src_surf.mem_type = GDDEPLOY_BUF_MEM_SYSTEM;
#elif WITH_TS
        src_surf.mem_type = GDDEPLOY_BUF_MEM_TS;
#elif WITH_RK
        src_surf.mem_type = GDDEPLOY_BUF_MEM_RK_RGA;
#else
        src_surf.mem_type = GDDEPLOY_BUF_MEM_SYSTEM;
#endif
        src_surf.batch_size = 1;
        src_surf.num_filled = 1;
#if WITH_NVIDIA
        src_surf.is_contiguous = 1;
#else
        src_surf.is_contiguous = 0;    // AVFrame的两个plane地址不一定连续
#endif

        BufSurfaceParams param;

        BufSurfacePlaneParams plane_param;
        plane_param.width[0] = in->width;
        plane_param.height[0] = in->height;
        plane_param.bytes_per_pix[0] = 1;

        if (in->format == AV_PIX_FMT_YUV420P || in->format == AV_PIX_FMT_YUVJ420P){
            plane_param.num_planes = 3; 
#if WITH_BM1684
            plane_param.psize[0] = in->linesize[4] * in->height;        
            plane_param.data_ptr[0] = (void *)in->data[4];
            plane_param.psize[1] = in->linesize[5] * in->height / 2;
            plane_param.data_ptr[1] = (void *)in->data[5];
            plane_param.psize[2] = in->linesize[6] * in->height / 2;
            plane_param.data_ptr[2] = (void *)in->data[6];
#else
            plane_param.psize[0] = in->linesize[0] * in->height;        
            plane_param.data_ptr[0] = (void *)in->data[0];
            plane_param.psize[1] = in->linesize[1] * in->height / 2;
            plane_param.data_ptr[1] = (void *)in->data[1];
            plane_param.psize[2] = in->linesize[2] * in->height / 2;
            plane_param.data_ptr[2] = (void *)in->data[2];
#endif
            plane_param.offset[0] = 0;
            plane_param.offset[1] = plane_param.psize[0];
            plane_param.offset[2] = plane_param.psize[0] + plane_param.psize[1];
        } else if (in->format == AV_PIX_FMT_NV12 || in->format == AV_PIX_FMT_NV21 || in->format == AV_PIX_FMT_CUDA){
            plane_param.num_planes = 2; 
#if WITH_BM1684
            plane_param.psize[0] = in->linesize[4] * in->height;        
            plane_param.data_ptr[0] = (void *)in->data[4];
            plane_param.psize[1] = in->linesize[5] * in->height / 2;        
            plane_param.data_ptr[1] = (void *)in->data[5];
#else 
            plane_param.psize[0] = in->linesize[0] * in->height;        
            plane_param.data_ptr[0] = (void *)in->data[0];
            plane_param.psize[1] = in->linesize[1] * in->height / 2;        
            plane_param.data_ptr[1] = (void *)in->data[1];
#endif
            plane_param.offset[0] = 0;
            plane_param.offset[1] = plane_param.psize[0];
        }
        

        param.plane_params = plane_param;
#if WITH_TS
        VIDEO_FRAME_INFO_S *pstFrameInfo = (VIDEO_FRAME_INFO_S *)dst_surf->surface_list[0].data_ptr;
        // auto t0 = std::chrono::high_resolution_clock::now();
        auto size = av_image_get_buffer_size((AVPixelFormat)in->format, in->width, in->height, 32);
        auto ret = av_image_copy_to_buffer((uint8_t*)pstFrameInfo->stVFrame.u64VirAddr[0], size, (const uint8_t *const *)in->data,
                        (const int *)in->linesize, (AVPixelFormat)in->format, in->width, in->height, 1);
        // auto t1 = std::chrono::high_resolution_clock::now();
        // printf("!!!!!!!!!ffmpeg copy time: %d us\n", std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
        param.color_format = convertFormat(in->format);
        param.data_size = AVFrame_GetSize(in);
        param.width = in->width;
        param.height = in->height;
        
        src_surf.surface_list = &param;
#else
#if WITH_BM1684
        param.data_ptr = in->data[4];
#else
        param.data_ptr = in->data[0];
#endif
        param.color_format = convertFormat(in->format);
        param.data_size = AVFrame_GetSize(in);
        param.width = in->width;
        param.height = in->height;
        param.pitch = in->linesize[0];
        src_surf.surface_list = &param;

        BufSurfaceCopy(&src_surf, dst_surf);
        // cudaMemcpy(dst_surf->surface_list[0].data_ptr, src_surf.surface_list[0].data_ptr, src_surf.surface_list[0].data_size, cudaMemcpyDeviceToDevice);
#endif
    }else{
        // BufSurfaceParams *src_param = &dst_surf->surface_list[0];
        // src_param->data_ptr = reinterpret_cast<void *>(in->data[4]);

#if WITH_BM1684
        dst_surf->mem_type = GDDEPLOY_BUF_MEM_BMNN;
#elif WITH_NVIDIA
        dst_surf->mem_type = GDDEPLOY_BUF_MEM_NVIDIA;
#elif WITH_RK
        dst_surf->mem_type = GDDEPLOY_BUF_MEM_RK_RGA;
#else
        dst_surf->mem_type = GDDEPLOY_BUF_MEM_SYSTEM;
#endif
        dst_surf->batch_size = 1;
        dst_surf->num_filled = 1;
#if WITH_NVIDIA
        dst_surf->is_contiguous = 1;    
#else
        dst_surf->is_contiguous = 0;    // AVFrame的两个plane地址不一定连续
#endif

        BufSurfaceParams *param = new BufSurfaceParams;

        BufSurfacePlaneParams plane_param;
        plane_param.width[0] = in->width;
        plane_param.height[0] = in->height;
        plane_param.bytes_per_pix[0] = 1;

        if (in->format == AV_PIX_FMT_YUV420P || in->format == AV_PIX_FMT_YUVJ420P){
            plane_param.num_planes = 3; 
#if WITH_BM1684
            plane_param.psize[0] = in->linesize[0] * in->height;        
            plane_param.data_ptr[0] = (void *)in->data[4];
            plane_param.psize[1] = in->linesize[1] * in->height / 2;
            plane_param.data_ptr[1] = (void *)in->data[5];
            plane_param.psize[2] = in->linesize[2] * in->height / 2;
            plane_param.data_ptr[2] = (void *)in->data[6];
#else
            plane_param.psize[0] = in->linesize[0] * in->height;        
            plane_param.data_ptr[0] = (void *)in->data[0];
            plane_param.psize[1] = in->linesize[1] * in->height / 2;
            plane_param.data_ptr[1] = (void *)in->data[1];
            plane_param.psize[2] = in->linesize[2] * in->height / 2;
            plane_param.data_ptr[2] = (void *)in->data[2];
#endif
            plane_param.offset[0] = 0;
            plane_param.offset[1] = plane_param.psize[0];
            plane_param.offset[2] = plane_param.psize[0] + plane_param.psize[1];
        } else if (in->format == AV_PIX_FMT_NV12 || in->format == AV_PIX_FMT_NV21){
            plane_param.num_planes = 2; 
#if WITH_BM1684
            plane_param.psize[0] = in->linesize[4] * in->height;        
            plane_param.data_ptr[0] = (void *)in->data[4];
            plane_param.psize[1] = in->linesize[5] * in->height / 2;        
            plane_param.data_ptr[1] = (void *)in->data[5];
#elif WITH_TS

#else 
            plane_param.psize[0] = in->linesize[0] * in->height;        
            plane_param.data_ptr[0] = (void *)in->data[0];
            plane_param.psize[1] = in->linesize[1] * in->height / 2;        
            plane_param.data_ptr[1] = (void *)in->data[1];
#endif
            plane_param.offset[0] = 0;
            plane_param.offset[1] = plane_param.psize[0];
        }
        

        param->plane_params = plane_param;
#if WITH_BM1684
        param->data_ptr = in->data[4];
#elif WITH_TS
        param->data_ptr =  (VIDEO_FRAME_INFO_S *)in->buf[0]->data;
#else 
        param->data_ptr = in->data[0];
#endif
        param->color_format = convertFormat(in->format);
        param->data_size = AVFrame_GetSize(in);
        param->width = in->width;
        param->height = in->height;
        
        dst_surf->surface_list = param;
#if WITH_TS
        dst_surf->mem_type = GDDEPLOY_BUF_MEM_TS;
#endif
    }

    return 0;
}

std::vector<cv::Mat> convertPackage2Mat(gddeploy::PackagePtr package)
{
    std::vector<cv::Mat> mats;
    #if WITH_NVIDIA
    for (uint32_t i = 0; i < package->data.size(); i++){
        auto data = package->data[i];
        auto surf = data->GetLref<gddeploy::BufSurfWrapperPtr>();
        BufSurface *surface = surf->GetBufSurface();
        std::vector<unsigned char> y_buffer(surface->surface_list[i].plane_params.height[0] * surface->surface_list[i].plane_params.pitch[0]);
        std::vector<unsigned char> uv_buffer((surface->surface_list[i].plane_params.height[0] / 2) * surface->surface_list[i].plane_params.pitch[1]);
        // 从 GPU 内存拷贝到 CPU 内存缓冲区
        cudaMemcpy(y_buffer.data(), surface->surface_list[i].plane_params.data_ptr[0], surface->surface_list[i].plane_params.height[0] * surface->surface_list[i].plane_params.pitch[0], cudaMemcpyDeviceToHost);
        cudaMemcpy(uv_buffer.data(), surface->surface_list[i].plane_params.data_ptr[1], surface->surface_list[i].plane_params.height[0] / 2 * surface->surface_list[i].plane_params.pitch[1], cudaMemcpyDeviceToHost);

        // 使用 CPU 内存缓冲区创建 cv::Mat，这里使用的是正确的步长
        cv::Mat y_plane(surface->surface_list[i].plane_params.height[0], surface->surface_list[i].plane_params.width[0], CV_8UC1, y_buffer.data(), surface->surface_list[i].plane_params.pitch[0]);
        cv::Mat uv_plane(surface->surface_list[i].plane_params.height[0] / 2, surface->surface_list[i].plane_params.width[0] / 2, CV_8UC1, uv_buffer.data(), surface->surface_list[i].plane_params.pitch[1]);
        // 现在可以将 Y 和 UV 数据转换成 BGR 格式
        cv::Mat bgr_mat;
        cv::cvtColorTwoPlane(y_plane, uv_plane, bgr_mat, cv::COLOR_YUV2BGR_NV12);
        // cv::imwrite(std::string("/volume1/code3/gddeploy/build/")+std::to_string(i)+std::string("_decode.jpg"), bgr_mat);
        mats.push_back(bgr_mat);
    }
    #endif
    return mats;
}

#ifdef WITH_RK
std::shared_ptr<AVFrame> VideoRunnerPriv::convertRkRgaMem2AVFrame(const std::shared_ptr<RkRgaMem> rkframe)
{
    AVFrame *avframe = av_frame_alloc();
    avframe->format = AV_PIX_FMT_NV12;
    avframe->width = rkframe->width;
    avframe->height = rkframe->height;
    avframe->linesize[0] = rkframe->hor_stride;
    avframe->linesize[1] = rkframe->hor_stride;
    avframe->data[0] = (uint8_t*)rkframe->mem_info->buf_;
    avframe->data[1] = (uint8_t*)rkframe->mem_info->buf_ + rkframe->hor_stride * rkframe->ver_stride;
    return std::shared_ptr<AVFrame>(avframe, [](AVFrame *p) { av_frame_free(&p); });
}
#endif
static int callbackCount = 0; 
int VideoRunnerPriv::OpenStream(std::string video_path, std::string save_path, bool is_draw)
{
    thread_infer_handle_ = std::thread([&, this]()
    {
        this->benchmake_startime = std::chrono::steady_clock::now();
        // for(int i = 0; i < 1000; i++)
        while(1)
        {
            BufSurfWrapperPtr surf_ptr = this->out_pool_surf_.wait_for_data();
            gddeploy::PackagePtr in = gddeploy::Package::Create(1);
            in->data[0]->Set(surf_ptr);
            infer_api_.InferAsync(in, [this](gddeploy::Status status, gddeploy::PackagePtr data, gddeploy::any user_data) mutable {
                //lock
                // std::unique_lock<std::mutex> lock(g_mutex);
                callbackCount++;
                // // 获取当前时间
                auto currentTime = std::chrono::steady_clock::now();
                // // 计算总时间
                std::chrono::duration<double> totalTime = std::chrono::duration_cast<std::chrono::duration<double>>(currentTime - this->benchmake_startime);
                // // 计算帧率
                double fps = callbackCount / totalTime.count();
                GDDEPLOY_INFO("InferAsync callbackCount {}, totalTime {}, fps {}", callbackCount, totalTime.count(), fps);
                gddeploy::BufSurfWrapperPtr surf_ptr = data->data[0]->GetLref<gddeploy::BufSurfWrapperPtr>();
                this->in_pool_surf_.push(surf_ptr);
            });
        }
    });

    thread_decode_frame_ = std::thread([&, this, video_path](){
        int frame_num = 0;
        decoder_ = std::make_shared<Decoder>();   
        auto last_time = std::chrono::high_resolution_clock::now(); // 初始时间戳
        std::chrono::duration<double, std::milli> total_duration(0); // 总时间初始化

#if WITH_RK
        if (-1 == decoder_->open_stream(video_path, [this, &frame_num, &last_time, &total_duration](const int64_t frame_idx, const std::shared_ptr<RkRgaMem> &rkframe) {
            InitPoolSurf(rkframe);
            std::shared_ptr<AVFrame> avframe = convertRkRgaMem2AVFrame(rkframe);
#else
        if (-1 == decoder_->open_stream(video_path, [this, &frame_num, &last_time, &total_duration](const int64_t frame_idx, const std::shared_ptr<AVFrame> &avframe) {
            InitPoolSurf(avframe);
#endif
            frame_num++;
            auto now = std::chrono::high_resolution_clock::now(); // 当前时间戳
            std::chrono::duration<double, std::milli> elapsed = now - last_time; // 计算时间差
            total_duration += elapsed; 
            double average_fps = 1000.0 * frame_num / total_duration.count();
            GDDEPLOY_INFO("Decode stream_id time taken for one frame: {} ms Average FPS: {} ms frame_num {}", elapsed.count(), average_fps, frame_num); // 打印时间差
            // if(average_fps > 15)
            // {
            //     return;
            // }
            last_time = now; // 更新时间戳
            
            BufSurfWrapperPtr surf_ptr = this->in_pool_surf_.wait_for_data();
            convertAVFrame2BufSurface1(avframe.get(), surf_ptr, true);
            this->out_pool_surf_.push(surf_ptr);
        })){
            GDDEPLOY_INFO("start decode stream_id fail");
        }else{
            GDDEPLOY_INFO("start decode stream_id success");
        }
        });
    return 0;
}

int VideoRunner::Init(const std::string config, std::vector<std::string> model_paths, std::vector<std::string> license_paths)
{
    return priv_->Init(config, model_paths, license_paths);
}

int VideoRunner::OpenStream(std::string video_path, std::string save_path, bool is_draw)
{
    return priv_->OpenStream(video_path, save_path, is_draw);
}

int VideoRunner::Join()
{
    return priv_->Join();
}

VideoRunner::VideoRunner()
{
    priv_ = std::make_shared<VideoRunnerPriv>();
}