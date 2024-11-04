#include "app/runner_pic.h"
#include "api/global_config.h"
#include "api/infer_api.h"
#include "common/logger.h"
#include "common/util.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include "CommandParser/CommandParser.h"

using namespace gddeploy;

// 读取raw rgb文件数据
int read_raw_rgb_file(const char *filename, unsigned char *data, uint32_t data_size, int width, int height)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("open file %s failed\n", filename);
        return -1;
    }

    if (fread(data, 1, data_size, fp) != data_size) {
        printf("read file %s failed\n", filename);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int ParseAndCheckArgs(int argc, const char *argv[], CommandParser &options)
{
    // Construct the command parser
    options.AddOption("--model", "./model.gdd", "Specify the first model file path, default: ./model.gdd");
    options.AddOption("--license", "",   "Specify the first model file path, default: ./license");
    options.AddOption("--raw-pic-path", "./test.jpg",  "Specify the model file path, default: ./test.jpg");
    options.AddOption("--img-format", "rgb",  "Specify the raw pic format, default: rgb");
    options.AddOption("--img-height", "0",  "Specify the raw pic height, default: 0");
    options.AddOption("--img-width", "0",  "Specify the raw pic width, default: 0");   
    options.AddOption("--save-path", "./",          "Specify the model file path, filename will be the same as input file, default: ./");

    options.ParseArgs(argc, argv);

    return 0;
}

int main(int argc, const char **argv)
{
    CommandParser options;
    // Parse and check arguments
    int ret = ParseAndCheckArgs(argc, argv, options);
    if (ret != 0) {
        GDDEPLOY_ERROR("ParseAndCheckArgs failed");
        return ret;
    }

    std::string model_path = options.GetStringOption("--model");
    std::string license_path = options.GetStringOption("--license");
    std::string pic_path = options.GetStringOption("--raw-pic-path");
    std::string save_path = options.GetStringOption("--save-path");
    int img_h = options.GetIntOption("--img-height");
    int img_w = options.GetIntOption("--img-width");
    std::string img_format = options.GetStringOption("--img-format");

    // print the arguments
    GDDEPLOY_DEBUG("------------------------parse args------------------------");
    GDDEPLOY_DEBUG("model_path: {}", model_path);
    GDDEPLOY_DEBUG("license_path: {}", license_path);
    GDDEPLOY_DEBUG("pic_path: {}", pic_path);
    GDDEPLOY_DEBUG("save_path: {}", save_path);
    GDDEPLOY_DEBUG("img_h: {}", img_h);
    GDDEPLOY_DEBUG("img_w: {}", img_w);
    GDDEPLOY_DEBUG("img_format: {}", img_format);
    GDDEPLOY_DEBUG("------------------------parse args------------------------");

    // 1. init gddeploy
    if ( 0 != gddeploy_init("")){
        GDDEPLOY_ERROR("[app] gddeploy init fail\n");
        return -1;
    }

    // 2. create infer api and Init, it will load model and create context
    // std::string config = "";
    // std::ifstream ifs("./config.json");
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
    std::string config = R"({
        "create_pipeline" : {
            "skip_processor" : ["preproc", "postproc"]
        }
    })";

    gddeploy::InferAPI infer_api;
    infer_api.Init(config, model_path, license_path, ENUM_API_PROCESSOR_API);

    // 3. prepare input package
    BufSurface *surface = new BufSurface();
    surface->mem_type = GDDEPLOY_BUF_MEM_SYSTEM;
    surface->batch_size = 1;
    surface->num_filled = 1;

    BufSurfacePlaneParams plane_param;
    // memset(&plane_param, 0, sizeof(BufSurfacePlaneParams));
    plane_param.num_planes = 1;
    plane_param.width[0] = img_w;
    plane_param.height[0] = img_h;

    BufSurfaceParams *param = new BufSurfaceParams();
    param->plane_params = plane_param;
    unsigned char *input_data = nullptr;
    if (img_format == "rgb") {
        input_data = new unsigned char[img_w * img_h * 3 * sizeof(float)] ;
        if (read_raw_rgb_file(pic_path.c_str(), input_data, img_w * img_h * 3 * sizeof(float), img_w, img_h) != 0) {
            GDDEPLOY_ERROR("read raw rgb file failed");
            return -1;
        }

        param->color_format = GDDEPLOY_BUF_COLOR_FORMAT_RGB;
        param->data_size = img_w * img_h * 3 * sizeof(float);
    } else if (img_format == "nv12") {

        input_data = new unsigned char[img_w * img_h * 3 / 2];
        if (read_raw_rgb_file(pic_path.c_str(), input_data, img_w * img_h * 3 / 2, img_w, img_h) != 0) {
            GDDEPLOY_ERROR("read raw nv12 file failed");
            return -1;
        }

        param->color_format = GDDEPLOY_BUF_COLOR_FORMAT_NV12;
        param->data_size = img_w * img_h * 3 / 2 * sizeof(char);
    } else {
        GDDEPLOY_ERROR("unsupported img format: {}", img_format);
        return -1;
    }
    param->width = img_w;
    param->height = img_h;
    param->data_ptr = (void*)(input_data);
    
    surface->surface_list = param;
    gddeploy::BufSurfWrapperPtr surf = std::make_shared<gddeploy::BufSurfaceWrapper>(surface, false);

    gddeploy::PackagePtr in = gddeploy::Package::Create(1);
    gddeploy::PackagePtr out = gddeploy::Package::Create(1);

    in->predict_io = std::make_shared<gddeploy::InferData>();
    in->predict_io->Set(std::move(surf));

    // 4. inference
    infer_api.InferSync(in, out);

    // 5. get result
    std::vector<BufSurfWrapperPtr> out_bufs = out->predict_io->GetLref<std::vector<BufSurfWrapperPtr>>();

    // 6. save result
    for (uint32_t i = 0; i < out_bufs.size(); i++) {
        BufSurface *out_surface = out_bufs[i]->GetBufSurface();
        int out_w = out_surface->surface_list[0].width;
        int out_h = out_surface->surface_list[0].height;
        int out_size = out_surface->surface_list[0].data_size;
        GDDEPLOY_INFO("out_w: {}, out_h: {}, out_size: {}", out_w, out_h, out_size);

        char *out_data = (char *)out_surface->surface_list[0].data_ptr;

        // save result to file
        std::string out_file_name = pic_path.substr(pic_path.find_last_of("/") + 1);
        out_file_name = out_file_name.substr(0, out_file_name.find_last_of(".")) + "_out" + std::to_string(i) + ".bin";
        std::string out_file_path = JoinPath(save_path, out_file_name);
        std::ofstream out_file(out_file_path, std::ios::out | std::ios::binary);
        out_file.write(out_data, out_size);
        out_file.close();
    }

    // 7. release resource
    delete[] input_data;
    delete surface;
    delete param;

    return 0;
}
