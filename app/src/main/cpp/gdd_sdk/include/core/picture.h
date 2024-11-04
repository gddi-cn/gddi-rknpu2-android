#pragma once

#include <string>
#include <vector>
#include "core/mem/buf_surface.h"
#include "core/mem/buf_surface_util.h"
#include "core/mem/buf_surface_impl.h"

namespace gddeploy
{

// Picture算子抽象，一般包含resize、crop、normalize、csc等操作
// 输入输出以BufSurface为单位
class Picture
{
public:
    static Picture *Instance() noexcept
    {
        if (pInstance_ == nullptr)
        {
            pInstance_ = new Picture();
        }
        return pInstance_;
    }

    int register_pic(std::string manufacturer, std::string chip, Picture *predictor_creator) noexcept
    {
        std::unordered_map<std::string, Picture *> chip_predictor_creator;
        chip_predictor_creator[chip] = predictor_creator;
        pic_map_[manufacturer] = chip_predictor_creator;

        return 0;
    }

    Picture *GetPicture(std::string manufacturer, std::string chip)
    {
        if (pic_map_.count(manufacturer) == 0)
            return nullptr;
        
        if (pic_map_[manufacturer].count("any") != 0)
            return pic_map_[manufacturer]["any"];
        return pic_map_[manufacturer][chip];
    }

    int Decode(std::string pic_file, gddeploy::BufSurfWrapperPtr &dst);
    int Encode(std::string pic_file, gddeploy::BufSurfWrapperPtr &dst, const int quality = 100);

    virtual int Decode(std::vector<uint8_t> &data, gddeploy::BufSurfWrapperPtr &dst){ return 0; }
    virtual int Encode(std::vector<uint8_t> &data, gddeploy::BufSurfWrapperPtr &dst, const int quality = 100){ return 0; }

private:
    std::unordered_map<std::string, std::unordered_map<std::string, Picture *>> pic_map_;

    static Picture *pInstance_;
};

int register_pic_module();

}  // namespace gddeploy