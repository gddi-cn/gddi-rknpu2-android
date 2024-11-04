#ifndef RESULT_DEF_C_H
#define RESULT_DEF_C_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LABEL_LEN 32
#define MAX_POINT_NUM 17

// Detection result
typedef struct {
    float x;
    float y;
    float w;
    float h;
} Bbox;

typedef struct {
    int x;
    int y;
    int number;
    float score;
} PoseKeyPoint;

typedef struct {
    // int label;
    int detect_id;
    int class_id;
    float score;
    Bbox bbox;
    PoseKeyPoint point[MAX_POINT_NUM];    //关键点
    char label[MAX_LABEL_LEN];
} DetectObject;  // struct DetectObject

typedef struct {
    int img_id;
    int img_w;
    int img_h;
    DetectObject *detect_objs;
    int num_detect_obj;
} DetectImg;

// class DetectResult : public InferResult{
typedef struct {
    uint32_t batch_size;
    DetectImg *detect_imgs;
    int detect_img_num;
} DetectResult;

#ifdef __cplusplus
}
#endif

#endif /* RESULT_DEF_C_H */