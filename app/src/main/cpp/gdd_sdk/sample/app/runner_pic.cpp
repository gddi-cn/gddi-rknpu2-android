#include "app/runner_pic.h"

#include <fstream>
#include <dirent.h>
#include <memory>
#include <sys/stat.h>

#include "api/infer_api.h"
#include "api/global_config.h"
// #include "app/endecode.h"
#include "app/result_handle.h"
#include "common/type_convert.h"
#include "common/logger.h"
#include "common/util.h"
#include "core/mem/buf_surface_util.h"
#include "core/picture.h"
#include "core/result_def.h"

#include "opencv2/opencv.hpp"

#if WITH_NVIDIA
#include "pic/nvjpeg.h"
#endif

namespace gddeploy {

static std::string GetFilename(std::string path)
{
    std::string::size_type iPos = path.find_last_of('/') + 1;
    std::string filename = path.substr(iPos, path.length() - 1);
    return filename;
}

class PicRunnerPriv{
public:
    PicRunnerPriv()=default;

    int Init(const std::string config, std::string model_path, std::string license = "");
    int Init(const std::string config, std::vector<std::string> model_paths, std::vector<std::string> license);

    int InferSync(std::string pic_path, std::string save_path="", bool is_draw=false);    //推理路径下所有图片
    int InferMultiPicSync(std::vector<std::string> pic_path, std::string save_path="", bool is_draw=false);    //推理路径下所有图片

    int InferAsync(std::string pic_path, std::string save_path="", bool is_draw=false);
    int InferSync(cv::Mat in_mat, gddeploy::InferResult& result);
private:
    InferAPI infer_api_;

    std::vector<InferAPI> infer_api_v_;

    any pic_hw_decoder_; //选择图片解码器

    any pic_hw_encoder_;//选择图片编码器
};

}

using namespace gddeploy;

int PicRunnerPriv::Init(const std::string config, std::string model_path, std::string license)
{
    if ( 0 != gddeploy_init("")){
        GDDEPLOY_ERROR("[app] gddeploy init fail\n");
        return -1;
    }

    if ( 0 != infer_api_.Init(config, model_path,license, ENUM_API_SESSION_API))
    {
        return -1;
    } 
    infer_api_v_.push_back(infer_api_);
    return 0;
}

int PicRunnerPriv::Init(const std::string config, std::vector<std::string> model_paths, std::vector<std::string> license_paths)
{
    if ( 0 != gddeploy_init("")){
        GDDEPLOY_ERROR("[app] gddeploy init fail\n");
        return -1;
    }

    for (uint32_t i = 0; i < model_paths.size(); i++){
        std::string model_path = model_paths[i];
        std::string license_path = license_paths[i];
        
        InferAPI infer_api;

        int ret = infer_api.Init(config, model_path, license_path, ENUM_API_SESSION_API);
        if (ret != 0){
            return ret;
        }
        
        infer_api_v_.push_back(infer_api);
    }
    // TODO: 判断该硬件是否有硬件编解码接口

    return 0;
}

int PicRunnerPriv::InferSync(cv::Mat in_mat, gddeploy::InferResult& result)
{
    // 2. 推理
    gddeploy::BufSurfWrapperPtr surf;
#if WITH_BM1684
    bm_image img;
    cv::bmcv::toBMI(in_mat, &img, true);
    convertBmImage2BufSurface(img, surf, false);
#else
    convertMat2BufSurface(in_mat, surf, true);
#endif

    gddeploy::PackagePtr in = gddeploy::Package::Create(1);
    in->data[0]->Set(surf);
    gddeploy::PackagePtr out = gddeploy::Package::Create(1);

    
    if (0 != infer_api_v_[0].InferSync(in, out))
    {
        GDDEPLOY_ERROR("infer failed");
        return -1;
    }

    if (false == out->data[0]->HasMetaValue())
    {
        GDDEPLOY_ERROR("infer result is empty");
        return -1;
    }
    result = out->data[0]->GetMetaData<gddeploy::InferResult>(); 
    return 0;
}

int PicRunnerPriv::InferSync(std::string pic_path, std::string save_path, bool is_draw)
{
    // 1. 图片解码
    // if(pic_hw_decoder_->is_support(pic_path)){  // 硬件支持可支持该图片编码格式，否则改为OpenCV软解

    // }else{
    //     //采用OpenCV解码
    // }
#if 0
    NvjpegImageProcessor jpg_processor;
    BufSurface surface;
    std::vector<std::string> pic_files = {pic_path};

    jpg_processor.Decode(pic_files, &surface);

    gddeploy::BufSurfWrapperPtr surf = std::make_shared<gddeploy::BufSurfaceWrapper>(&surface, false);
#else
    cv::Mat in_mat = cv::imread(pic_path);

    // 2. 推理
    gddeploy::BufSurfWrapperPtr surf;
#if WITH_BM1684
    bm_image img;
    cv::bmcv::toBMI(in_mat, &img, true);
    convertBmImage2BufSurface(img, surf, false);
#else
    convertMat2BufSurface(in_mat, surf, true);
#endif

#endif
    gddeploy::PackagePtr in = gddeploy::Package::Create(1);
    in->data[0]->Set(surf);
    gddeploy::PackagePtr out = gddeploy::Package::Create(1);

    if(infer_api_v_.size() == 2 && infer_api_v_[0].GetModelType() == "pose" && infer_api_v_[1].GetModelType() == "face_recognition")
    {
        gddeploy::PackagePtr in = gddeploy::Package::Create(1);
        in->data[0]->Set(surf);
        gddeploy::PackagePtr out = gddeploy::Package::Create(1);
        infer_api_v_[0].InferSync(in, out);
        gddeploy::PackagePtr in_face_recognition = gddeploy::Package::Create(1);
        gddeploy::PackagePtr out_face_recognition = gddeploy::Package::Create(1);
        for(uint32_t i = 0; i < out->data.size(); i++)
        {
            gddeploy::InferResult result = out->data[i]->GetMetaData<gddeploy::InferResult>();
            PrintResult(result);
            in_face_recognition->data[i]->Set(surf);
            in_face_recognition->data[i]->SetMetaData(std::move(result));

        }
        infer_api_v_[1].InferSync(in_face_recognition, out_face_recognition);
        for(uint32_t i = 0; i < out_face_recognition->data.size(); i++)
        {
            gddeploy::InferResult result = out_face_recognition->data[i]->GetMetaData<gddeploy::InferResult>();
            PrintResult(result);
        }
    }
    else if (infer_api_v_.size() > 1){
        for (auto &infer_api : infer_api_v_){
            // 循环将上一模型结果传入下一模型的输入
            if (true == out->data[0]->HasMetaValue()){
                // in->data[0]->SetMetaData(std::move(out->data[0]->GetMetaData<gddeploy::InferResult>()));
                in->data[0] = std::move(out->data[0]);
            }
            
            infer_api.InferSync(in, out);
            if (false == out->data[0]->HasMetaValue())
                break;
            
            gddeploy::InferResult result = out->data[0]->GetMetaData<gddeploy::InferResult>();

            PrintResult(result);
            // 3. 编码输出
            if (is_draw && save_path != ""){
            // if(pic_hw_encoder_->is_support(pic_path)){  // 硬件支持可支持该图片编码格式，否则改为OpenCV软解

            // }else{
            //     //采用OpenCV编码
            // }
                auto pic_paths = std::vector<std::string>{pic_path};
                auto surfs = std::vector<gddeploy::BufSurfWrapperPtr>{};
                auto in_mats = std::vector<cv::Mat>{};
                std::string img_save_path = JoinPath(save_path, GetFilename(pic_path));

                DrawBbox(result, pic_paths, in_mats, surfs, img_save_path);
            }
        }
    } else {
        infer_api_.InferSync(in, out);

        if (false == out->data[0]->HasMetaValue())
            return -1;

        gddeploy::InferResult result = out->data[0]->GetMetaData<gddeploy::InferResult>(); 
        PrintResult(result);

        // 3. 编码输出
        if (is_draw && save_path != ""){
        // if(pic_hw_encoder_->is_support(pic_path)){  // 硬件支持可支持该图片编码格式，否则改为OpenCV软解

        // }else{
        //     //采用OpenCV编码
        // }
            auto pic_paths = std::vector<std::string>{pic_path};
            auto surfs = std::vector<gddeploy::BufSurfWrapperPtr>{};
            auto in_mats = std::vector<cv::Mat>{};
            std::string img_save_path = JoinPath(save_path, GetFilename(pic_path));
            DrawBbox(result, pic_paths, in_mats, surfs, img_save_path);
        }
    }

    return 0;
}

int PicRunnerPriv::InferMultiPicSync(std::vector<std::string> pic_path, std::string save_path, bool is_draw)
{
    // 1. 图片解码
    // if(pic_hw_decoder_->is_support(pic_path)){  // 硬件支持可支持该图片编码格式，否则改为OpenCV软解

    // }else{
    //     //采用OpenCV解码
    // }
    int pic_num = pic_path.size();

    const int MAX_PER_TIME_SIZE = 1; // 一次最多推理64张图片

    for (int per_time_start = 0; per_time_start < pic_num; per_time_start += MAX_PER_TIME_SIZE) {
        int per_time_size = std::min(MAX_PER_TIME_SIZE, pic_num - per_time_start);
        std::vector<cv::Mat> in_mats;
        std::vector<gddeploy::BufSurfWrapperPtr> surfs;
        std::vector<std::string> pic_paths;

        gddeploy::PackagePtr in = gddeploy::Package::Create(per_time_size);
        gddeploy::BufSurfWrapperPtr surf;
        for (int i = 0; i < per_time_size; i++) {
            int idx = per_time_start + i; 
        #if 0
            NvjpegImageProcessor jpg_processor;
            BufSurface surface;
            std::vector<std::string> pic_files = {pic_path};

            jpg_processor.Decode(pic_files, &surface);

            gddeploy::BufSurfWrapperPtr surf = std::make_shared<gddeploy::BufSurfaceWrapper>(&surface, false);
        #else
            cv::Mat in_mat = cv::imread(pic_path[idx]);
            if(in_mat.empty())
                continue;

            // 2. 推理
            
        #if WITH_BM1684
            bm_image img;
            cv::bmcv::toBMI(in_mat, &img, true);
            convertBmImage2BufSurface(img, surf, false);
        #else
            convertMat2BufSurface(in_mat, surf, true);
        #endif

        #endif
            
            in->data[i]->Set(surf);


            in_mats.push_back(in_mat);
            surfs.push_back(surf);
            pic_paths.push_back(pic_path[idx]);
        }

        gddeploy::PackagePtr out = gddeploy::Package::Create(1);

        if (infer_api_v_.size() > 0){
            for (auto &infer_api : infer_api_v_){
                // 循环将上一模型结果传入下一模型的输入
                if (true == out->data[0]->HasMetaValue())
                    in->data[0]->SetMetaData(out->data[0]->GetMetaData<gddeploy::InferResult>());

                // infer_api.InferSync(in, out);
                infer_api.InferAsync(in, [](gddeploy::Status status, gddeploy::PackagePtr data, gddeploy::any user_data){

                    });
                #if 0
                if (false == out->data[0]->HasMetaValue())
                    break;
                
                for (int i = 0; i < in_mats.size(); i++){
                    gddeploy::InferResult result = out->data[i]->GetMetaData<gddeploy::InferResult>();

                    PrintResult(result);
                    // 3. 编码输出
                    if (is_draw && save_path != ""){
                    // if(pic_hw_encoder_->is_support(pic_path)){  // 硬件支持可支持该图片编码格式，否则改为OpenCV软解

                    // }else{
                    //     //采用OpenCV编码
                    // }
                        auto pic_paths_tmp = std::vector<std::string>{pic_paths[i]};
                        auto surfs_tmp = std::vector<gddeploy::BufSurfWrapperPtr>{surfs[i]};
                        auto in_mats_tmp = std::vector<cv::Mat>{in_mats[i]};

                        std::string img_save_path = save_path + GetFilename(pic_paths[i]);

                        DrawBbox(result, pic_paths_tmp, in_mats_tmp, surfs_tmp, img_save_path);
                    }
                }
                #endif
            }
        }
        else {
            infer_api_.InferSync(in, out);

            if (false == out->data[0]->HasMetaValue())
            return -1;

            gddeploy::InferResult result = out->data[0]->GetMetaData<gddeploy::InferResult>(); 
            PrintResult(result);

            // 3. 编码输出
            if (is_draw && save_path != ""){
            // if(pic_hw_encoder_->is_support(pic_path)){  // 硬件支持可支持该图片编码格式，否则改为OpenCV软解

            // }else{
            //     //采用OpenCV编码
            // }
                auto pic_paths = std::vector<std::string>{pic_path};
                auto surfs = std::vector<gddeploy::BufSurfWrapperPtr>{};
                auto in_mats = std::vector<cv::Mat>{};
                std::string img_save_path = save_path + GetFilename(pic_paths[0]);

                DrawBbox(result, pic_paths, in_mats, surfs, img_save_path);
            }
        }
    }
    return 0;
}

int PicRunnerPriv::InferAsync(std::string pic_path, std::string save_path, bool is_draw)
{
    //可一次解码多张图再推理，增大batch
    return 0;
}


PicRunner::PicRunner(){
    priv_ = std::make_shared<PicRunnerPriv>();
}

int PicRunner::Init(const std::string config, std::string model_path, std::string license)
{
    int ret = priv_->Init(config, model_path,license);
    return ret;
}

int PicRunner::Init(const std::string config, std::vector<std::string> model_paths, std::vector<std::string> license)
{
    int ret = priv_->Init(config, model_paths, license);
    return ret;
}

int PicRunner::InferSync(cv::Mat int_mat,gddeploy::InferResult &result)
{
    gddeploy::BufSurfWrapperPtr surf;
    convertMat2BufSurface(int_mat, surf, true);
    gddeploy::PackagePtr in = gddeploy::Package::Create(1);
    in->data[0]->Set(surf);
    gddeploy::PackagePtr out = gddeploy::Package::Create(1);
    priv_->InferSync(int_mat, result);
    if (false == out->data[0]->HasMetaValue())
        return -1;
    return 0;
}

int PicRunner::InferSync(std::string pic_path, std::string save_path, bool is_draw)
{
    // 查询是否图片还是图片路径，获取全部图片
    std::vector<std::string> imgs_path;
    struct stat s_buf;
    stat(pic_path.c_str(), &s_buf);
    if (S_ISDIR(s_buf.st_mode)){
        struct dirent * ptr;
        DIR *dir = opendir((char *)pic_path.c_str()); //打开一个目录
        while((ptr = readdir(dir)) != NULL) //循环读取目录数据
        {
            std::string filename(ptr->d_name, strlen(ptr->d_name));
        
            if (filename == "." || filename == "..")
                continue;
            
            std::string extension = filename.substr(filename.find_last_of(".") + 1);
            if (extension != "jpg" && extension != "png" && extension != "JPG") {
                continue;
            }
            
            imgs_path.push_back(pic_path + filename);
        }
        closedir(dir);
    }else{
        imgs_path.push_back(pic_path);
    }

    // 循环异步推理图片数据
    // if (imgs_path.size() > 1){
    //     priv_->InferMultiPicSync(imgs_path, save_path, is_draw);
    // } else {
    //     priv_->InferSync(imgs_path[0], save_path, is_draw);
    // }
    for (auto img_path : imgs_path){
        if (0 != priv_->InferSync(img_path, save_path, is_draw)){
            GDDEPLOY_ERROR("[app] InferSync fail\n");
            continue;
        }
    }
    
    return 0;
}

int PicRunner::InferAsync(std::string pic_path, std::string save_path, bool is_draw)
{
        // 查询是否图片还是图片路径，获取全部图片
    std::vector<std::string> imgs_path;
    struct stat s_buf;
    stat(pic_path.c_str(), &s_buf);
    if (S_ISDIR(s_buf.st_mode)){
        struct dirent * ptr;
        DIR *dir = opendir((char *)pic_path.c_str()); //打开一个目录
        while((ptr = readdir(dir)) != NULL) //循环读取目录数据
        {
            if (std::string(ptr->d_name) == "." || std::string(ptr->d_name) == "..")
                continue;
            imgs_path.push_back(JoinPath(pic_path, std::string(ptr->d_name)));
        }
    }else{
        imgs_path.push_back(pic_path);
    }

    // 循环异步推理图片数据
    for (auto img_path : imgs_path){
        std::string img_save_path = save_path + GetFilename(img_path);

        if (0 != priv_->InferAsync(img_save_path, save_path, is_draw)){
            GDDEPLOY_ERROR("[app] InferAsync fail\n");
            continue;
        }
    }
    return 0;
}

