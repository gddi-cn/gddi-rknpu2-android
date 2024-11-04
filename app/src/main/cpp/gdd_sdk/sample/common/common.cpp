#include "common.h"
#include <map>
#include <thread>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <cstdlib>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include "common/logger.h"

namespace gddeploy
{
    cv::Rect2i GetPreProcParam(BufSurfaceParams *in_surf_param, BufSurfaceParams *out_surf_param,std::string net_type)
    {
        int w, h, x, y;
        float r_w = out_surf_param->width / (in_surf_param->width * 1.0);
        float r_h = out_surf_param->height / (in_surf_param->height * 1.0);
        if (r_h > r_w)
        {
            w = out_surf_param->width;
            h = r_w * in_surf_param->height;
            x = 0;
            if (net_type == "yolox"|| net_type == "ocr_rec" ||
                net_type == "OCRNet")
                y = 0;
            else
                y = (out_surf_param->height - h) / 2;
        }
        else
        {
            w = r_h * in_surf_param->width;
            h = out_surf_param->height;
            if (net_type == "yolox"|| net_type == "ocr_rec" ||
                net_type == "OCRNet")
                x = 0;
            else
                x = (out_surf_param->width - w) / 2;
            y = 0;
        }
        if (net_type == "arcface" ||
            net_type == "dolg")
        {
            w = out_surf_param->width;
            h = out_surf_param->height;
            x = 0;
            y = 0;
        }
        return cv::Rect2i(x, y, w, h);
    }

    int Align(int value, int align_bits) {
        return value % align_bits ? (value / align_bits + 1) * align_bits : value;
    }

    std::map<std::pair<std::thread::id, std::string>, std::chrono::high_resolution_clock::time_point> timeMap;
    std::mutex mtx;
    // #define TIC_EXPORT 0
    void TIC(const std::string& label)
    {
    #if TIC_EXPORT
        return;
    #endif

        std::lock_guard<std::mutex> lock(mtx);
        timeMap[std::make_pair(std::this_thread::get_id(), label)] = std::chrono::high_resolution_clock::now();
    }

    void TOC(const std::string& label)
    {
    #if TIC_EXPORT
        return;
    #endif
        std::lock_guard<std::mutex> lock(mtx);
        auto t2 = std::chrono::high_resolution_clock::now();
        auto t1 = timeMap[std::make_pair(std::this_thread::get_id(), label)];

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
        // printf("%vocab Duration: %ld milliseconds.\n", label.c_str(), duration);
        GDDEPLOY_INFO("{} Duration: {} milliseconds.\n", label.c_str(), duration);
    }

    void Bgr2Nv12(cv::Mat &bgr, cv::Mat &nv12)
    {
        int image_h = bgr.rows;
        int image_w = bgr.cols;

        // 转换到 I420
        cv::Mat I420;
        cv::cvtColor(bgr, I420, cv::COLOR_BGR2YUV_I420);

        uint8_t *pI420 = I420.ptr<uint8_t>();
        int I420_y_s = image_h * image_w;
        int I420_uv_h = image_h / 2;
        int I420_uv_w = image_w / 2;

        // 准备 NV12 输出 Mat
        nv12 = cv::Mat(image_h * 3 / 2, image_w, CV_8UC1);
        uint8_t *pNV12 = nv12.ptr<uint8_t>();

        // 拷贝 I420 中的 Y 数据到 NV12 中
        std::memcpy(pNV12, pI420, I420_y_s);

        // 设置 UV 指针
        uint8_t *pI420_U = pI420 + I420_y_s;
        uint8_t *pI420_V = pI420_U + I420_uv_h * I420_uv_w;
        pNV12 = pNV12 + I420_y_s;

        // 遍历 I420 中 UV 分量数据，复制到 NV12 中
        for(int i = 0; i < I420_uv_h * I420_uv_w; i++)
        {
            *pNV12 = *pI420_U;
            pNV12++;
            pI420_U++;

            *pNV12 = *pI420_V;
            pNV12++;
            pI420_V++;
        }
    }

    // std::wstring GetCharacterFromVocab(const std::string &vocab) {
    //     // boost::locale::generator gen;
    //     // std::locale loc = gen("");

    //     // std::wstring wide_vocab = boost::locale::conv::to_utf<wchar_t>(vocab, loc);

    //     // return wide_vocab;
    // }

    // std::string WStringToString(const std::wstring &wide_str) {
    //     // boost::locale::generator gen;
    //     // std::locale loc = gen("");

    //     // std::string utf8_str = boost::locale::conv::from_utf(wide_str, loc);

    //     // return utf8_str;
    // }

    std::wstring GetCharacterFromVocab(const std::string &vocab) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring result = converter.from_bytes(vocab);
        return result;
    }

    std::string WStringToString(const std::wstring &wide_str) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::string result = converter.to_bytes(wide_str);
        return result;
    }

} // namespace gddeploy