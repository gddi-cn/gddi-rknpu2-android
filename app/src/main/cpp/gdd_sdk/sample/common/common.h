#ifndef GDDEPLOY_COMMON_H
#define GDDEPLOY_COMMON_H
#include <opencv2/core.hpp>
#include "core/mem/buf_surface.h"

namespace gddeploy
{
    cv::Rect2i GetPreProcParam(BufSurfaceParams *in_surf_param, BufSurfaceParams *out_surf_param,std::string net_type);
    int Align(int value, int align_bits);
    void TIC(const std::string& label = "");
    void TOC(const std::string& label = "");
    void Bgr2Nv12(cv::Mat &bgr, cv::Mat &nv12);
    std::wstring GetCharacterFromVocab(const std::string &vocab);
    std::string WStringToString(const std::wstring &wide_str);
} // namespace gddeploy

#endif // COMMON_H