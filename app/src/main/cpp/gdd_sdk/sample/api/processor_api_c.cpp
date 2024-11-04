#include "api/infer_api.h"
#include <memory>
#include <string>
#include "common/logger.h"
#include "core/model.h"
#include "core/pipeline.h"
#include "api/processor_api.h"
#include "api/session_api.h"
#include "core/result_def.h"
#include "api/global_config.h"
#include "opencv2/opencv.hpp"
#include "common/type_convert.h"

namespace gddeploy {

    class GddObserver : public gddeploy::Observer {
    public:
        GddObserver() {}

        GddObserver(InferAsyncCallback cb) {
            callback_ = cb;
        }

        void Response(gddeploy::Status status, gddeploy::PackagePtr out, gddeploy::any user_data) noexcept {
            if (callback_ != nullptr) {
                callback_(status, out, user_data);
            }
        }

        void SetCallback(InferAsyncCallback cb) {
            callback_ = cb;
        }

    private:
        InferAsyncCallback callback_ = nullptr;
    };


    class InferAPICPrivate {
    public:
        InferAPICPrivate();

        ~InferAPICPrivate();

        int Init(const std::string config, const std::string model_path, std::string license = "",
                 ENUM_API_TYPE api_type = ENUM_API_PROCESSOR_API);

        int InferSync(const gddeploy::PackagePtr &in, gddeploy::PackagePtr &out);

        int InferAsync(const gddeploy::PackagePtr &in, int timeout = 0);

        int WaitTaskDone(const std::string &tag);

        std::vector<std::string> GetLabels();

        void SetCallback(InferAsyncCallback cb) {
            // observer_ = std::make_shared<GddObserver>(cb);
            // infer_server_->SetObserver(session_async_, observer_);
            api_sess_.SetCallback(cb);
            cb_ = cb;
        }

        std::string GetPreProcConfig();

        std::string GetModelType();

        Picture *GetPicture();

        bool IsSupportHwPicDecode();

        int dev_id_;
        int stream_id_;
    private:
        ENUM_API_TYPE api_type_;

        // ProcessorAPI 相关
        ProcessorAPI processor_api_;
        std::vector<ProcessorPtr> processors_;

        // SessionAPI 相关
        InferAsyncCallback cb_;
        SessionAPI api_sess_;
    };


    InferAPICPrivate::InferAPICPrivate() {
    }

    InferAPICPrivate::~InferAPICPrivate() {
    }

    int InferAPICPrivate::Init(const std::string config, const std::string model_path, std::string license,
                               ENUM_API_TYPE api_type) {
        if (0 != gddeploy_init("")) {
            GDDEPLOY_ERROR("[app] gddeploy init fail\n");
            return -1;
        }

        api_type_ = api_type;
        if (api_type == ENUM_API_SESSION_API) {
            return api_sess_.Init(config, model_path, "", license);
        } else if (api_type == ENUM_API_PROCESSOR_API) {
            if (0 != processor_api_.Init(config, model_path, license)) {
                printf("ProcessorAPI init failed\n");
                return -1;
            }
            processors_ = processor_api_.GetProcessor();
        }

        return 0;
    }

    int InferAPICPrivate::InferSync(const gddeploy::PackagePtr &in, gddeploy::PackagePtr &out) {
        if (api_type_ == ENUM_API_SESSION_API) {
            bool ret = api_sess_.InferSync(in, out);
            if (ret) {
                GDDEPLOY_ERROR("infer server request sync false");
                return -1;
            }
        } else if (api_type_ == ENUM_API_PROCESSOR_API) {
            // 4. 循环执行每个processor的Process函数
            for (auto processor: processors_) {
                if (gddeploy::Status::SUCCESS != processor->Process(in))
                    break;
            }
            out = in;
        }
        return 0;
    }

    int InferAPICPrivate::InferAsync(const gddeploy::PackagePtr &in, int timeout) {
        bool ret = api_sess_.InferAsync(in, cb_, timeout);
        if (ret) {
            GDDEPLOY_ERROR("infer server request sync false");
            return -1;
        }

        return 0;
    }

    int InferAPICPrivate::WaitTaskDone(const std::string &tag) {
        api_sess_.WaitTaskDone(tag);
        return 0;
    }

    std::string InferAPICPrivate::GetModelType() {
        std::string model_type = "";
        if (api_type_ == ENUM_API_SESSION_API) {
            model_type = api_sess_.GetModelType();
        } else if (api_type_ == ENUM_API_PROCESSOR_API) {
            model_type = processor_api_.GetModelType();
        }

        return model_type;
    }

    std::vector<std::string> InferAPICPrivate::GetLabels() {
        std::vector<std::string> labels;

        if (api_type_ == ENUM_API_SESSION_API) {
            labels = api_sess_.GetLabels();
        } else if (api_type_ == ENUM_API_PROCESSOR_API) {
            labels = processor_api_.GetLabels();
        }

        return labels;
    }

    Picture *InferAPICPrivate::GetPicture() {
        Picture *picture = nullptr;

        if (api_type_ == ENUM_API_SESSION_API) {
            picture = api_sess_.GetPicture();
        } else if (api_type_ == ENUM_API_PROCESSOR_API) {
            picture = processor_api_.GetPicture();
        }

        return picture;
    }

    bool InferAPICPrivate::IsSupportHwPicDecode() {
        bool is_picture = false;

        if (api_type_ == ENUM_API_SESSION_API) {
            is_picture = api_sess_.IsSupportHwPicDecode();
        } else if (api_type_ == ENUM_API_PROCESSOR_API) {
            is_picture = processor_api_.IsSupportHwPicDecode();
        }

        return is_picture;
    }

    std::string InferAPICPrivate::GetPreProcConfig() {
        std::string config = "";

        if (api_type_ == ENUM_API_SESSION_API) {
            config = api_sess_.GetPreProcConfig();
        } else if (api_type_ == ENUM_API_PROCESSOR_API) {
            config = processor_api_.GetPreProcConfig();
        }

        return config;
    }
}   // namespace gddeploy

#include "capi/processor_api_c.h"

#ifdef __cplusplus
extern "C" {
#endif

int Init(void **handle, const char *config_file, const char *model_path, const char *license_file) {

    gddeploy::InferAPICPrivate *priv_ = new gddeploy::InferAPICPrivate();

    // 读取config_file文件

    std::string config = "";
    // std::ifstream ifs(config_file);
    // if (ifs.is_open()){
    //     std::string line;
    //     while (getline(ifs, line)){
    //         config += line + "\n";
    //     }
    //     ifs.close();
    // }else{
    //     printf("config file open failed\n");
    //     delete priv_;
    //     return -1;
    // }


    if (priv_->Init(config, model_path, license_file) != 0) {
        printf("Init failed\n");
        delete priv_;
        return -1;
    }

    *handle = (void *) priv_;

    return 0;
}


int InitJna(void **handle, const char *config, const char *model_path, const char *license_file) {
    gddeploy::InferAPICPrivate *priv_ = new gddeploy::InferAPICPrivate();

    if (priv_->Init(config, model_path, license_file) != 0) {
        printf("Init failed\n");
        delete priv_;
        return -1;
    }

    *handle = (void *) priv_;

    return 0;
}

int Deinit(void *handle) {
    gddeploy::InferAPICPrivate *priv_ = (gddeploy::InferAPICPrivate *) handle;
    delete priv_;
    return 0;
}

int GetLabels(void *handle, char **output_labels, int *output_label_size) {
    gddeploy::InferAPICPrivate *priv_ = (gddeploy::InferAPICPrivate *) handle;
    auto labels = priv_->GetLabels();

    *output_label_size = labels.size();
    *output_labels = (char *) malloc(labels.size() * 100);

    for (uint32_t i = 0; i < labels.size(); i++) {
        std::string label = labels[i];
        strncpy(*output_labels + i * 100, label.c_str(), 100);
    }

    return 0;
}

int GetPreProcConfig(void *handle, char **output_config, int *output_config_size) {
    gddeploy::InferAPICPrivate *priv_ = (gddeploy::InferAPICPrivate *) handle;
    std::string config = priv_->GetPreProcConfig();

    *output_config_size = config.size();

    *output_config = (char *) malloc(config.size() + 1);

    strncpy(*output_config, config.c_str(), config.size());

    return 0;
}

int InferJna(void *handle, const char *input_data, int input_size, BufSurfaceColorFormat format, int img_w, int img_h,
             DetectResult *output_result) {
    gddeploy::InferAPICPrivate *priv_ = (gddeploy::InferAPICPrivate *) handle;
    if (input_size != img_w * img_h * 3 || format != GDDEPLOY_BUF_COLOR_FORMAT_BGR) {
//        GDDEPLOY_ERROR("only input GDDEPLOY_BUF_COLOR_FORMAT_BGR\n");
        return -1;
    }
    cv::Mat in_mat(img_h, img_w, CV_8UC3, (void *) input_data);

    gddeploy::BufSurfWrapperPtr surf;
    convertMat2BufSurface(in_mat, surf, true);
    gddeploy::PackagePtr in = gddeploy::Package::Create(1);
    in->data[0]->Set(surf);
    gddeploy::PackagePtr out;
    priv_->InferSync(in, out);

    // 取出数据和保存
    auto labels = priv_->GetLabels();

    const gddeploy::InferResult &postproc_results = out->data[0]->GetMetaData<gddeploy::InferResult>();
    for (auto result_type: postproc_results.result_type) {
        if (result_type == gddeploy::GDD_RESULT_TYPE_DETECT) {

            output_result->batch_size = 1;
            output_result->detect_img_num = 1;

            output_result->detect_imgs = (DetectImg *) malloc(
                    output_result->detect_img_num * sizeof(DetectImg));//new DetectImg[1];
            output_result->detect_imgs[0].img_id = 0;
            output_result->detect_imgs[0].img_w = img_w;
            output_result->detect_imgs[0].img_h = img_h;
            output_result->detect_imgs[0].num_detect_obj = postproc_results.detect_result.detect_imgs[0].detect_objs.size();
            // output_result->detect_imgs[0].detect_objs = new DetectObject[output_result->detect_imgs[0].num_detect_obj];
            output_result->detect_imgs[0].detect_objs = (DetectObject *) malloc(
                    output_result->detect_imgs[0].num_detect_obj * sizeof(DetectObject));

            for (uint32_t i = 0; i < postproc_results.detect_result.detect_imgs[0].detect_objs.size(); i++) {
                auto obj = postproc_results.detect_result.detect_imgs[0].detect_objs[i];

                output_result->detect_imgs[0].detect_objs[i].detect_id = i;
                output_result->detect_imgs[0].detect_objs[i].class_id = obj.class_id;
                output_result->detect_imgs[0].detect_objs[i].bbox.x = obj.bbox.x;
                output_result->detect_imgs[0].detect_objs[i].bbox.y = obj.bbox.y;
                output_result->detect_imgs[0].detect_objs[i].bbox.w = obj.bbox.w;
                output_result->detect_imgs[0].detect_objs[i].bbox.h = obj.bbox.h;
                output_result->detect_imgs[0].detect_objs[i].score = obj.score;

                if (labels[obj.class_id].size() < MAX_LABEL_LEN) {
                    strncpy((char *) output_result->detect_imgs[0].detect_objs[i].label, labels[obj.class_id].c_str(),
                            labels[obj.class_id].size());
                    output_result->detect_imgs[0].detect_objs[i].label[labels[obj.class_id].size()] = '\0';
                }
            }
        }
    }

    return 0;
}

int Infer(void *handle, const char *input_data, int input_size, int ori_img_w, int ori_img_h, int img_w, int img_h,
          DetectResult *output_result) {
    gddeploy::InferAPICPrivate *priv_ = (gddeploy::InferAPICPrivate *) handle;
    // if (cb != nullptr)
    //     priv_->SetCallback(cb);
    // if (-1 == priv_->InferAsync(in, timeout))
    //     return -1;

    // 准备数据
    BufSurface *surface = new BufSurface();
    surface->mem_type = GDDEPLOY_BUF_MEM_SYSTEM;
    surface->batch_size = 1;
    surface->num_filled = 1;

    BufSurfacePlaneParams plane_param;
    plane_param.num_planes = 1;
    plane_param.width[0] = img_w;
    plane_param.height[0] = img_h;

    BufSurfaceParams *param = new BufSurfaceParams();
    param->plane_params = plane_param;
    param->color_format = GDDEPLOY_BUF_COLOR_FORMAT_NV12;
    param->data_size = img_w * img_h * 3 / 2 * sizeof(char);
    param->width = img_w;
    param->height = img_h;
    param->data_ptr = (void *) (input_data);

    surface->surface_list = param;
    gddeploy::BufSurfWrapperPtr surf = std::make_shared<gddeploy::BufSurfaceWrapper>(surface, false);

    gddeploy::PackagePtr in = gddeploy::Package::Create(1);

    gddeploy::FrameInfo frame_info = {0};
    frame_info.width = ori_img_w;
    frame_info.height = ori_img_h;
    in->data[0]->SetFrameData(frame_info);

    in->predict_io = std::make_shared<gddeploy::InferData>();
    in->predict_io->Set(std::move(surf));

    gddeploy::PackagePtr out;
    priv_->InferSync(in, out);

    // 取出数据和保存
    auto labels = priv_->GetLabels();

    const gddeploy::InferResult &postproc_results = out->data[0]->GetMetaData<gddeploy::InferResult>();
    for (auto result_type: postproc_results.result_type) {
        if (result_type == gddeploy::GDD_RESULT_TYPE_DETECT) {

            output_result->batch_size = 1;
            output_result->detect_img_num = 1;

            output_result->detect_imgs = (DetectImg *) malloc(
                    output_result->detect_img_num * sizeof(DetectImg));//new DetectImg[1];
            output_result->detect_imgs[0].img_id = 0;
            output_result->detect_imgs[0].img_w = img_w;
            output_result->detect_imgs[0].img_h = img_h;
            output_result->detect_imgs[0].num_detect_obj = postproc_results.detect_result.detect_imgs[0].detect_objs.size();
            // output_result->detect_imgs[0].detect_objs = new DetectObject[output_result->detect_imgs[0].num_detect_obj];
            output_result->detect_imgs[0].detect_objs = (DetectObject *) malloc(
                    output_result->detect_imgs[0].num_detect_obj * sizeof(DetectObject));

            for (uint32_t i = 0; i < postproc_results.detect_result.detect_imgs[0].detect_objs.size(); i++) {
                auto obj = postproc_results.detect_result.detect_imgs[0].detect_objs[i];

                output_result->detect_imgs[0].detect_objs[i].detect_id = i;
                output_result->detect_imgs[0].detect_objs[i].class_id = obj.class_id;
                output_result->detect_imgs[0].detect_objs[i].bbox.x = obj.bbox.x;
                output_result->detect_imgs[0].detect_objs[i].bbox.y = obj.bbox.y;
                output_result->detect_imgs[0].detect_objs[i].bbox.w = obj.bbox.w;
                output_result->detect_imgs[0].detect_objs[i].bbox.h = obj.bbox.h;
                output_result->detect_imgs[0].detect_objs[i].score = obj.score;

                if (labels[obj.class_id].size() < MAX_LABEL_LEN) {
                    strncpy((char *) output_result->detect_imgs[0].detect_objs[i].label, labels[obj.class_id].c_str(),
                            labels[obj.class_id].size());
                    output_result->detect_imgs[0].detect_objs[i].label[labels[obj.class_id].size()] = '\0';
                }
            }
        }
    }

    return 0;
}


#ifdef __cplusplus
}
#endif