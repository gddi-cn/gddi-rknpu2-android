#ifndef GDDEPLOY_CONFIG_H
#define GDDEPLOY_CONFIG_H

namespace gddeploy
{
    
/*
配置信息
UserConfig：用户额外设置参数
ModelConfig：解析模型得到的模型参数信息
*/

} // namespace gddeploy

// 定义模型名字
#define MODEL_CLASSIFY "classification"
#define MODEL_DETECT "detection"
#define MODEL_SEGMENT "segmentation"
#define MODEL_POSE "pose"
#define MODEL_FACE "face_recognition"
#define MODEL_ACTION "action"
#define MODEL_IMAGE_RETRIEVAL "image_retrieval"
#define MODEL_OCR "ocr"
#define MODEL_ROTATED_DETECT "rotated_detection"

// 定义网络名字
#define MODEL_CLASSIFY_NET_OFA "ofa"
#define MODEL_DETECT_NET_YOLOV5 "yolo"
#define MODEL_DETECT_NET_YOLOV6 "yolov6"
#define MODEL_SEGMENT_NET_OCRNET "OCRNet"
#define MODEL_POSE_NET_KEYPOINT "yolox"  // yolox
#define MODEL_POSE_NET_RTMPOSE "rtmpose"  // rtmpose
#define MODEL_FACE_NET_ARCFACE "arcface"
#define MODEL_ACTION_NET_ACTION "tsn_gddi"
#define MODEL_IMAGE_RETRIEVAL_NET_DOLG "dolg"
#define MODEL_OCR_NET_RESNET31V2CTC "resnet31v2ctc"
#define MODEL_OCR_NET_REC "ocr_rec"
#define MODEL_OCR_NET_DET "ocr_det"
#define MODEL_CLASSIFY_NET_GDDINAS "gddinas"
#define MODEL_CLASSIFY_NET_MULTIMODAL "multimodal"
#define MODEL_ROTATED_DETECT_NET_RTMDET "rtmdet"

#endif