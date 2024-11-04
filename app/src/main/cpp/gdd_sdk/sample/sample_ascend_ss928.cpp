#include <iostream>
#include <sstream>
#include <string>

#include "api/global_config.h"
#include "api/processor_api.h"
#include "api/infer_api.h"
#include "common/logger.h"

#include "CommandParser/CommandParser.h"

using namespace gddeploy;

int ParseAndCheckArgs(int argc, const char *argv[], CommandParser &options)
{
    // Construct the command parser
    options.AddOption("--model", "./model.gdd", "Specify the model file path, default: ./model.gdd");
    options.AddOption("--license", "",   "Specify the model file path, default: ./license");
    options.AddOption("--pic-path", "./test.jpg",  "Specify the model file path, default: ./test.jpg");
    options.AddOption("--save-path", "./",          "Specify the model file path, default: ./");
    options.ParseArgs(argc, argv);

    return 0;
}

// read raw image
int ReadRawImage(const std::string &pic_path, uint8_t *&data, int &width, int &height)
{
    // read raw image
    FILE *fp = fopen(pic_path.c_str(), "rb");
    if (fp == nullptr) {
        GDDEPLOY_ERROR("Failed to open file: {}", pic_path);
        return -1;
    }

    // read header
    uint8_t header[12];
    fread(header, 1, 12, fp);
    if (header[0] != 'Y' || header[1] != 'U' || header[2] != 'Y' || header[3] != 'V' ||
        header[4] != 0  || header[5] != 0  || header[6] != 0  || header[7] != 0  ||
        header[8] != 0  || header[9] != 0  || header[10]!= 0  || header[11]!= 0) {
        GDDEPLOY_ERROR("Invalid YUV420P header");
        fclose(fp);
        return -1;
    }

    // read width and height
    width = (header[12] << 24) | (header[13] << 16) | (header[14] << 8) | header[15];
    height = (header[16] << 24) | (header[17] << 16) | (header[18] << 8) | header[19];

    // read data
    int size = width * height * 3 / 2;
    data = new uint8_t[size];
    fread(data, 1, size, fp);
    fclose(fp);

    return 0;
}

int main(int argc, const char **argv)
{
    std::vector<std::string> model_paths;
    std::vector<std::string> license_paths;
    std::string pic_path;
    std::string save_path;

    CommandParser options;
    // Parse and check arguments
    int ret = ParseAndCheckArgs(argc, argv, options);
    if (ret != 0) {
        GDDEPLOY_ERROR("ParseAndCheckArgs failed");
        return ret;
    }

    std::string model_tmp = options.GetStringOption("--model");
    if (model_tmp != "") {
        // 判断字符串中是否有字符“;”，如果有就分割字符串赋值
        if (model_tmp.find(",") != std::string::npos) {
            std::stringstream ss(model_tmp);
            std::string item;
            while(std::getline(ss, item, ',')) {
                model_paths.push_back(item);
            }
        } else {
            model_paths.push_back(model_tmp);
        }
    } else {
        model_paths.push_back("");
    }

    std::string license_tmp = options.GetStringOption("--license");
    if (license_tmp != "") {
        // 判断字符串中是否有字符“;”，如果有就分割字符串赋值
        if (license_tmp.find(",") != std::string::npos) {
            std::stringstream ss(license_tmp);
            std::string item;
            while(std::getline(ss, item, ',')) {
                license_paths.push_back(item);
            }
        } else {
            license_paths.push_back(license_tmp);
        }
    } else {
        license_paths.push_back("");
    }
    
    pic_path = options.GetStringOption("--pic-path");
    save_path = options.GetStringOption("--save-path");

    // print the arguments
    GDDEPLOY_DEBUG("------------------------parse args------------------------");
    for (auto &item : model_paths) {
        GDDEPLOY_DEBUG("model_paths: {}", item);
    }
    for (auto &item : license_paths) {
        GDDEPLOY_DEBUG("license_paths: {}", item);
    }
    GDDEPLOY_DEBUG("pic_path: {}", pic_path);
    GDDEPLOY_DEBUG("save_path: {}", save_path);
    GDDEPLOY_DEBUG("------------------------parse args------------------------");

    // global config init, must be called before any other api and only once
    if ( 0 != gddeploy_init("")){
        GDDEPLOY_ERROR("[app] gddeploy init fail\n");
        return -1;
    }

    // "create_pipeline" : {
    //         "skip_processor" : ["preproc"],
    //     },
    std::string config = R"({
        "create_pipeline" : {
            "skip_processor" : ["preproc"],
        }
    })";

    // create infer_api
    gddeploy::InferAPI infer_api;
    if ( 0 != infer_api.Init(config, model_paths[0], license_paths[0], ENUM_API_PROCESSOR_API)){
        GDDEPLOY_ERROR("[app] gddeploy infer_api init fail\n");
        return -1;
    }

    // read raw image, image must be in yuv420p format, width and height must be multiple of 16
    uint8_t *data = nullptr;
    int width = 0;
    int height = 0;
    if ( 0 != ReadRawImage(pic_path, data, width, height)){
        GDDEPLOY_ERROR("[app] read raw image fail\n");
        return -1;
    }

    infer_api.InferSync(data, width, height, save_path);

    return 0;
}
