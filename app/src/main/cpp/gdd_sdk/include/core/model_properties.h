#ifndef GDDEPLOY_MODEL_PROPERTIES_H
#define GDDEPLOY_MODEL_PROPERTIES_H

#include <vector>
#include <string>
#include <memory>

namespace gddeploy
{
    // 平台提供的模型信息
    class ModelProperties
    {
    public:
        ModelProperties(std::string raw_info);

        int GetInputWidth() { return input_size_w_; }
        int GetInputHeight() { return input_size_h_; }
        int SetInputWidth(int width) { return input_size_w_ = width; }
        int SetInputHeight(int height) { return input_size_h_ = height; }

        std::string GetInputDest() { return input_dest_; }
        std::string GetForwardType() { return chip_ip_; }
        std::string GetChipType() { return chip_type_; }
        std::string GetProductType() { return product_type_; }
        std::string GetNetType() { return net_type_; }
        std::string GetModelType() { return model_type_; }
        float GetConfidenceThresh() { return conf_thresh_; }
        std::vector<std::string> GetLabels() { return labels_; }
        
        std::vector<float> GetAnchors() { return anchors_; }
        bool GetQat() { return qat_; }

        int GetSliceWidth() { return slice_w_; }
        int GetSliceHeight() { return slice_h_; }

        int GetInputSize() { return input_size_; }

        bool GetNeedClip() { return need_clip; }

        std::vector<float> GetAlignPoint() { return align_point_; }
        float* GetMean() { return mean_; }
        float* GetStd() { return std_; }
        void SetMean(float* mean) { mean_[0] = mean[0]; mean_[1] = mean[1]; mean_[2] = mean[2]; }
        void SetStd(float* std) { std_[0] = std[0]; std_[1] = std[1]; std_[2] = std[2]; }

        std::wstring GetWideVocab() { return wide_vocab_; }

        float GetPointSize() { return point_size_; }
    private:
        // TODO:存放解析后的各种数据
        std::string model_type_;   // detect/classify
        std::string net_type_;     // ssd/retina/yolo
        std::string product_type_; // Huawei
        std::string chip_type_;    // kirin710
        std::string chip_ip_;      // CPU/GPU/NPU
        float mean_[3];
        float std_[3];
        std::vector<std::string> labels_;
        std::string input_dest_; // RGB/YUV
        int input_size_h_;       // 320/640/224
        int input_size_w_;       // 320/640/224
        int input_size_;
        float conf_thresh_ = 0.5;      // 0.5
        std::vector<float> anchors_;
        bool qat_ = false; // qat true/false

        std::vector<float> align_point_;

        int slice_w_ = 0;
        int slice_h_ = 0;

        bool need_clip = false;
        std::wstring wide_vocab_;
        float point_size_ = 112.0;
    };

    using ModelPropertiesPtr = std::shared_ptr<ModelProperties>;

}

#endif