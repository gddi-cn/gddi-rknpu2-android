#include "app/runner_pic.h"
#include "api/global_config.h"
#include "api/infer_api.h"
#include "common/logger.h"

#include <iostream>
#include <sstream>

#include "CommandParser/CommandParser.h"

int ParseAndCheckArgs(int argc, const char *argv[], CommandParser &options)
{
    // Construct the command parser
    options.AddOption("--first-model", "./model.gdd", "Specify the first model file path, default: ./model.gdd");
    options.AddOption("--first-license", "",   "Specify the first model file path, default: ./license");
    options.AddOption("--secondary-model", "./model.gdd", "Specify the secondary model file path, default: ./model.gdd");
    options.AddOption("--secondary-license", "",   "Specify the secondary model file path, default: ./license");
    options.AddOption("--pic-path", "./test.jpg",  "Specify the model file path, default: ./test.jpg");
    options.AddOption("--save-path", "./",          "Specify the model file path, filename will be the same as input file, default: ./");
    options.ParseArgs(argc, argv);

    return 0;
}

int main(int argc, const char **argv)
{
    std::string first_model_path;
    std::string first_license_path;
    std::string secondary_model_path;
    std::string secondary_license_path;
    std::string pic_path;
    std::string save_path;

    CommandParser options;
    // Parse and check arguments
    int ret = ParseAndCheckArgs(argc, argv, options);
    if (ret != 0) {
        GDDEPLOY_ERROR("ParseAndCheckArgs failed");
        return ret;
    }

    first_model_path = options.GetStringOption("--first-model");
    first_license_path = options.GetStringOption("--first-license");
    secondary_model_path = options.GetStringOption("--secondary-model");
    secondary_license_path = options.GetStringOption("--secondary-license");
    
    pic_path = options.GetStringOption("--pic-path");
    save_path = options.GetStringOption("--save-path");

    // print the arguments
    GDDEPLOY_DEBUG("------------------------parse args------------------------");
    GDDEPLOY_DEBUG("first_model_path: {}", first_model_path);
    GDDEPLOY_DEBUG("first_license_path: {}", first_license_path);
    GDDEPLOY_DEBUG("secondary_model_path: {}", secondary_model_path);
    GDDEPLOY_DEBUG("secondary_license_path: {}", secondary_license_path);
    GDDEPLOY_DEBUG("pic_path: {}", pic_path);
    GDDEPLOY_DEBUG("save_path: {}", save_path);
    GDDEPLOY_DEBUG("------------------------parse args------------------------");

    // 1. init gddeploy
    if ( 0 != gddeploy_init("")){
        GDDEPLOY_ERROR("[app] gddeploy init fail\n");
        return -1;
    }




    return 0;
}
