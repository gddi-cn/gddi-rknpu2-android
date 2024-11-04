#include "app/runner_dataset.h"
#include "app/runner_pic.h"
#include "api/global_config.h"
#include "core/infer_server.h"
#include "common/logger.h"
#include "common/json.hpp"

#include <iostream>
#include <sstream>

#include "CommandParser/CommandParser.h"

int ParseAndCheckArgs(int argc, const char *argv[], CommandParser &options)
{
    // Construct the command parser
    options.AddOption("--model", "./model.gdd", "Specify the model file path, default: ./model.gdd");
    options.AddOption("--license", "./license",   "Specify the model file path, default: ./license");
    options.AddOption("--anno-file", "./anno.txt",  "Specify the model file path, default: ./anno.txt");
    options.AddOption("--pic-path", "./test.jpg",  "Specify the model file path, default: ./test.jpg");
    options.AddOption("--result-path", "./result.json",  "Specify the model file path, default: ./result.json");
    options.AddOption("--save-path", "./",          "Specify the model file path, default: ./");
    options.AddOption("--batch-size", "4",          "batch-size default: 4");

    options.ParseArgs(argc, argv);

    return 0;
}

int main(int argc, const char **argv)
{
    std::vector<std::string> model_paths;
    std::vector<std::string> license_paths;
    std::string anno_path;
    std::string pic_path;
    std::string result_path;
    std::string save_path;
    unsigned int batch_size;

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
    }

    anno_path = options.GetStringOption("--anno-file");
    pic_path = options.GetStringOption("--pic-path");
    result_path = options.GetStringOption("--result-path");
    save_path = options.GetStringOption("--save-path");
    batch_size = options.GetIntOption("--batch-size");

    // print the arguments
    GDDEPLOY_DEBUG("------------------------parse args------------------------");
    for (auto &item : model_paths) {
        GDDEPLOY_DEBUG("model_paths: {}", item);
    }
    for (auto &item : license_paths) {
        GDDEPLOY_DEBUG("license_paths: {}", item);
    }
    GDDEPLOY_DEBUG("anno_path: {}", anno_path);
    GDDEPLOY_DEBUG("pic_path: {}", pic_path);
    GDDEPLOY_DEBUG("result_path: {}", result_path);
    GDDEPLOY_DEBUG("save_path: {}", save_path);
    GDDEPLOY_DEBUG("------------------------parse args------------------------");

    gddeploy::DatasetRunner runner;

    nlohmann::json json_obj;
    json_obj["model_param"]["batch_size"] = batch_size;

    // Convert the JSON object to a string (optional, if you need to print or use the JSON as a string)
    std::string json_str = json_obj.dump();
    runner.Init(json_str, model_paths, license_paths);

    runner.InferSync(anno_path, pic_path, result_path, save_path, true);
    return 0;
}