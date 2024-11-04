#include "app/runner_pic.h"
#include "api/global_config.h"
#include "common/logger.h"
#include "common/json.hpp"
#include "opencv2/opencv.hpp"
#include <iostream>
#include <sstream>
#include "capi/processor_api_c.h"
#include "CommandParser/CommandParser.h"


int ParseAndCheckArgs(int argc, const char *argv[], CommandParser &options)
{
    // Construct the command parser
    options.AddOption("--model", "./model.gdd", "Specify the model file path, default: ./model.gdd");
    options.AddOption("--license", "./license",   "Specify the model file path, default: ./license");
    options.AddOption("--pic-path", "./test.jpg",  "Specify the model file path, default: ./test.jpg");
    options.AddOption("--save-path", "./",          "Specify the model file path, default: ./");
    options.AddOption("--batch-size", "4",          "batch-size default: 4");
    options.ParseArgs(argc, argv);

    return 0;
}

int main(int argc, const char **argv)
{
    std::vector<std::string> model_paths;
    std::vector<std::string> license_paths;
    std::string pic_path;
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
            // license_paths.push_back("");
        }
    }
    
    pic_path = options.GetStringOption("--pic-path");
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
    GDDEPLOY_DEBUG("pic_path: {}", pic_path);
    GDDEPLOY_DEBUG("save_path: {}", save_path);
    GDDEPLOY_DEBUG("------------------------parse args------------------------");

    

    nlohmann::json json_obj;
    json_obj["model_param"]["batch_size"] = batch_size;

    // Convert the JSON object to a string (optional, if you need to print or use the JSON as a string)
    std::string json_str = json_obj.dump();

    void *handle;
    if (0 != InitJna(&handle, "", model_paths[0].c_str(), license_paths[0].c_str())) {
        GDDEPLOY_ERROR("Init failed");
        return -1;    
    }
    cv::Mat img = cv::imread(pic_path);
    if (img.empty()) {
        GDDEPLOY_ERROR("read image failed");
        return -1;
    }
    DetectResult result;
    if(0 != InferJna(handle, (char*)img.data, img.cols * img.rows * img.channels(), GDDEPLOY_BUF_COLOR_FORMAT_BGR, img.cols, img.rows, &result)) {
        GDDEPLOY_ERROR("Infer failed");
        return -1;
    }
    //draw the result
    for (int i = 0; i < result.detect_img_num; i++) {
        DetectImg detect_img = result.detect_imgs[i];
        for (int j = 0; j < detect_img.num_detect_obj; j++) {
            DetectObject detect_obj = detect_img.detect_objs[j];
            cv::rectangle(img, cv::Rect(detect_obj.bbox.x, detect_obj.bbox.y, detect_obj.bbox.w, detect_obj.bbox.h), cv::Scalar(0, 255, 0), 2);
            cv::putText(img, detect_obj.label, cv::Point(detect_obj.bbox.x, detect_obj.bbox.y), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
        }
    }
    cv::imwrite(save_path + "/result.jpg", img);
    if (0 != Deinit(handle)) {
        GDDEPLOY_ERROR("Release failed\n");
        return -1;
    }

    return 0;
}
