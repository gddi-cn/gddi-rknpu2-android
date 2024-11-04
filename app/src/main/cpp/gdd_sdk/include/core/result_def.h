#pragma once


#include <fstream>
#include <dirent.h>
#include <iostream>
#include <vector>
#include <memory>

// #include "cnis/contrib/video_helper.h"
#include "core/infer_server.h"
#include "core/processor.h"

namespace gddeploy{
#define MAX_LABEL_LEN 32
// Classify result
typedef struct {
    int detect_id;
    int class_id;
    float score;
    std::string label;
}ClassifyObject;

typedef struct {
    int img_id;
    std::vector<ClassifyObject> detect_objs;
}ClassifyImg;

typedef struct{
    uint32_t batch_size = 0;
    std::vector<ClassifyImg> detect_imgs;

    void PrintResult(){
        std::cout << "Classify result: " << std::endl; 
        for (auto detect_img : detect_imgs) {
            std::cout << "Img id: " << detect_img.img_id << std::endl; 
            for (auto obj : detect_img.detect_objs) {
                std::cout << "class id: " << obj.class_id  << ", score: " << obj.score << ", label: " << obj.label << std::endl;
            }
        }
    }
}ClassifyResult;

using ActionResult = ClassifyResult;

// Detection result
struct Bbox{
    float x;
    float y;
    float w;
    float h;
    Bbox() : x(0.0f), y(0.0f), w(0.0f), h(0.0f) {}
    Bbox(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
};

struct DetectObject {
  // int label;
  int detect_id;            
  int class_id;         //类别id
  float score;          //检测得分
  Bbox bbox;            //检测框位置 bbox_[0]: x bbox_[1]: y bbox_[2]: w bbox_[3]: h
  std::string label;    //类别标签
  // Constructor
  DetectObject() : detect_id(0), class_id(0), score(0.0f), label("") {
    // Initialize bbox with zeros
    bbox.x = 0.0f;
    bbox.y = 0.0f;
    bbox.w = 0.0f;
    bbox.h = 0.0f;
  }
};  

typedef struct {
    int img_id;
    int img_w;
    int img_h;
    std::vector<DetectObject> detect_objs;
}DetectImg;

// class DetectResult : public InferResult{
typedef struct{
    uint32_t batch_size = 0;
    std::vector<DetectImg> detect_imgs;
    
    void PrintResult() { 
        std::cout << "Detect result: " << std::endl; 
        for (auto detect_img : detect_imgs) {
            std::cout << "Img id: " << detect_img.img_id << std::endl; 
            for (auto obj : detect_img.detect_objs) {
                std::cout << "detect id: " << obj.detect_id << " name: " << obj.label
                    << " class id: " << obj.class_id  << " score: " << obj.score
                    << " box[" << obj.bbox.x << ", " << obj.bbox.y << ", " << obj.bbox.w << ", " << obj.bbox.h << "]" << std::endl;
            }
        }
    }
}DetectResult;

typedef struct {
  int class_id;
  float score;
  float bbox[5];    //bbox_[0]: center_x bbox_[1]: center_y bbox_[2]: width bbox_[3]:height bbox_[4]: angle
  std::string label;

}RotatedDetectObject;  // struct DetectObject

typedef struct {
    int img_id;
    int img_w;
    int img_h;
    std::vector<RotatedDetectObject> rotated_detect_objs;
}RotatedDetectImg;

typedef struct{
    uint32_t batch_size = 0;
    std::vector<RotatedDetectImg> rotated_detect_imgs;
    
    void PrintResult() { 
        std::cout << "Detect result: " << std::endl; 
        for (auto rotated_detect_img : rotated_detect_imgs) {
            std::cout << "Img id: " << rotated_detect_img.img_id << std::endl; 
            for (auto obj : rotated_detect_img.rotated_detect_objs) {
                std::cout << " class id: " << obj.class_id  << " score: " << obj.score
                    << " box[" << obj.bbox[0] << ", " << obj.bbox[1] << ", " << obj.bbox[2] << ", " << obj.bbox[3] << ", " << obj.bbox[4] << "]" << std::endl;
            }
        }
    }
}RotatedDetectResult;


struct PoseKeyPoint{
    int x;
    int y;
    int number;
    float score;
};


// DetectPose result
typedef struct {
  // int label;
  int detect_id;
  int class_id;
  float score;
  Bbox bbox;
  std::vector<PoseKeyPoint> point;    //关键点
}DetectPoseObject;  // struct DetectObject

typedef struct {
    int img_id;
    int img_w;
    int img_h;
    std::vector<DetectPoseObject> detect_objs;
}DetectPoseImg;

// class DetectResult : public InferResult{
typedef struct{
    uint32_t batch_size = 0;
    std::vector<DetectPoseImg> detect_imgs;
    
    void PrintResult() { 
        std::cout << "Detect pose result: " << std::endl; 
        for (auto detect_img : detect_imgs) {
            std::cout << "Img id: " << detect_img.img_id << std::endl; 
            for (auto obj : detect_img.detect_objs) {
                std::cout << "detect id: " << obj.detect_id
                    << " class id: " << obj.class_id  << " score: " << obj.score
                    << " box[" << obj.bbox.x << ", " << obj.bbox.y << ", " << obj.bbox.w << ", " << obj.bbox.h << "]" << std::endl;
            }
        }
    }
}DetectPoseResult;

// Seg result
struct SegImg {
    int img_id;
    int map_w;
    int map_h;
    std::vector<uint8_t> seg_map;
};  // struct SegImg

typedef struct {
    uint32_t batch_size = 0;
    std::vector<SegImg> seg_imgs;

    void PrintResult(){
        std::cout << "Seg result: " << std::endl; 
        // 这里只需要把结果保存为.npy文件即可
        std::string pic_name = "./test.npy";

        std::ofstream ofile(pic_name);
        if(ofile.is_open()==false){
            return;
        }
        ofile.write((char*)seg_imgs[0].seg_map.data(), (seg_imgs[0].map_h * seg_imgs[0].map_w)*sizeof(float));
        ofile.close();
    }
}SegResult;

struct ImageRetrieval {
    int img_id;
    std::vector<float> feature;
};   // struct ImageRetrieval

typedef struct {
    uint32_t batch_size = 0;
    std::vector<ImageRetrieval> image_retrieval_imgs;

    void PrintResult(){
        std::cout << "Image retrieval result: " << std::endl; 
        for (auto image_retrieval_img : image_retrieval_imgs) {
            std::cout << "Img id: " << image_retrieval_img.img_id << std::endl; 
            std::cout << "Featrue id: [";
            for (auto feature : image_retrieval_img.feature) {
                std::cout << feature << ", ";
            }
            std::cout << "];" << std::endl;
        }
    }
}ImageRetrievalResult;

struct FaceRetrieval {
    int img_id;
    std::vector<float> feature;
};   // struct FaceRetrieval

typedef struct {
    uint32_t batch_size = 0;
    std::vector<FaceRetrieval> face_retrieval_imgs;

    void PrintResult(){
        std::cout << "Face retrieval result: " << std::endl; 
        for (auto face_retrieval_img : face_retrieval_imgs) {
            std::cout << "Img id: " << face_retrieval_img.img_id << std::endl; 
            std::cout << "Featrue id: [";
            for (auto feature : face_retrieval_img.feature) {
                std::cout << feature << ", ";
            }
            std::cout << "];" << std::endl;
        }
    }
}FaceRetrievalResult;


// OCR detection result
typedef struct {
  // int label;
  int detect_id;
  int class_id;
  float score;
  Bbox bbox;
  std::vector<PoseKeyPoint> point;    //关键点
}OcrDetectObject;  // struct DetectObject

typedef struct {
    int img_id;
    int img_w;
    int img_h;
    std::vector<OcrDetectObject> ocr_objs;
}OcrDetectImg;

// class DetectResult : public InferResult{
typedef struct{
    uint32_t batch_size = 0;
    std::vector<OcrDetectImg> ocr_detect_imgs;
    
    void PrintResult() { 
        std::cout << "Ocr detect result: " << std::endl; 
        for (auto detect_img : ocr_detect_imgs) {
            std::cout << "Img id: " << detect_img.img_id << std::endl; 
            for (auto obj : detect_img.ocr_objs) {
                std::cout << "detect id: " << obj.detect_id
                    << " class id: " << obj.class_id  << " score: " << obj.score
                    << " box[" << obj.bbox.x << ", " << obj.bbox.y << ", " << obj.bbox.w << ", " << obj.bbox.h << "]" << std::endl;
            }
        }
    }
}OcrDetectResult;

typedef struct {
  int class_id;
  float score;
  std::string name;  //分类名称
}OcrChar;

typedef struct {
  // int label;
  int detect_id;
  int class_id;
  float score;
  Bbox bbox;
  std::vector<PoseKeyPoint> point;    //关键点
  std::vector<OcrChar> chars;    //每个字符识别结果
  std::string chars_str;    //组合后字符串
}OcrRecObject;  // struct DetectObject

typedef struct {
    int img_id;
    int img_w;
    int img_h;
    std::vector<OcrRecObject> ocr_rec_objs;
}OcrRecImg;

// class DetectResult : public InferResult{
typedef struct{
    uint32_t batch_size = 0;
    std::vector<OcrRecImg> ocr_rec_imgs;
    
    void PrintResult() { 
        std::cout << "Ocr retrieval result: " << std::endl; 
        for (auto detect_img : ocr_rec_imgs) {
            std::cout << "Img id: " << detect_img.img_id << std::endl; 
            for (auto obj : detect_img.ocr_rec_objs) {
                std::cout << "detect id: " << obj.detect_id
                    << " class id: " << obj.class_id  << " score: " << obj.score
                    << " box[" << obj.bbox.x << ", " << obj.bbox.y << ", " << obj.bbox.w << ", " << obj.bbox.h << "]" 
                    << " chars: " << obj.chars_str
                    << std::endl;
            }
        }
    }
}OcrRecResult;


enum e_result_type_e{
    GDD_RESULT_TYPE_CLASSIFY = 0,
    GDD_RESULT_TYPE_DETECT,
    GDD_RESULT_TYPE_POSE,
    GDD_RESULT_TYPE_DETECT_POSE,
    GDD_RESULT_TYPE_SEG,
    GDD_RESULT_TYPE_ACTION,
    GDD_RESULT_TYPE_IMAGE_RETRIEVAL,
    GDD_RESULT_TYPE_FACE_RETRIEVAL,
    GDD_RESULT_TYPE_OCR_DETECT,
    GDD_RESULT_TYPE_OCR_RETRIEVAL,
    GDD_RESULT_TYPE_ROTATED_DETECT,
};

// class DetectResult;
typedef struct {
    std::vector<int> result_type;
    DetectResult detect_result;
    DetectPoseResult detect_pose_result;
    ClassifyResult classify_result;
    ActionResult action_result;
    SegResult seg_result;
    ImageRetrievalResult image_retrieval_result;
    FaceRetrievalResult face_retrieval_result;
    OcrDetectResult ocr_detect_result;
    OcrRecResult ocr_rec_result;
    RotatedDetectResult rotated_detect_result;
    void *user_data;
}InferResult;
}