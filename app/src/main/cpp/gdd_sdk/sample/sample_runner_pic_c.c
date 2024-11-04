#include "capi/processor_api_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>


// 读取raw rgb文件数据
int read_raw_rgb_file(const char *filename, unsigned char *data, int data_size, int width, int height)
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

int parse_pre_proc_config_data(char *pre_proc_config_data, char **align, int *height, int *width, char **interpolation)
{
    // 解析pre_proc_config_data，得到的配置信息如下：
    // {"preproc":{"bgr2rgb":true,"normalize":{"mean":[0.0,0.0,0.0],"std":[255.0,255.0,255.0]},"resize":{"align":"CENTER","height":640,"interpolation":"BILINEAR","width":640}}}
    // 其中，"bgr2rgb":true表示输入图片为BGR格式，需要转换为RGB格式；"normalize"表示对输入图片进行归一化，"mean"表示均值，"std"表示标准差；"resize"表示对输入图片进行resize，"align"表示对齐方式，"height"表示高度，"interpolation"表示插值方式，"width"表示宽度。

    // 解析pre_proc_config_data，得到resize配置信息
    char *resize_config = strstr(pre_proc_config_data, "resize");
    if (resize_config == NULL) {
        printf("parse pre_proc_config_data failed, no resize config\n");
        return -1;
    }
    resize_config = strstr(resize_config, "{");

    if (resize_config == NULL) {
        printf("parse pre_proc_config_data failed, no resize config\n");
        return -1;
    }

    char *align_config = strstr(resize_config, "align");
    if (align_config == NULL) {
        printf("parse pre_proc_config_data failed, no align config\n");
        return -1;
    }

    align_config = strstr(align_config, ":");
    align_config += 1;
    char *end_align_config = strstr(align_config, ",");
    *align = malloc(end_align_config - align_config + 1);
    strncpy(*align, align_config, end_align_config - align_config);
    (*align)[end_align_config - align_config] = '\0';

    char *height_config = strstr(resize_config, "height");
    if (height_config == NULL) {
        printf("parse pre_proc_config_data failed, no height config\n");
        return -1;
    }

    height_config = strstr(height_config, ":");
    height_config += 1;
    char *end_height_config = strstr(height_config, ",");
    *height = atoi(height_config);

    char *width_config = strstr(resize_config, "width");
    if (width_config == NULL) {
        printf("parse pre_proc_config_data failed, no width config\n");
        return -1;
    }

    width_config = strstr(width_config, ":");
    width_config += 1;
    char *end_width_config = strstr(width_config, "}");
    *width = atoi(width_config);

    char *interpolation_config = strstr(resize_config, "interpolation");
    if (interpolation_config == NULL) {
        printf("parse pre_proc_config_data failed, no interpolation config\n");
        return -1;
    }

    interpolation_config = strstr(interpolation_config, ":");
    interpolation_config += 1;
    char *end_interpolation_config = strstr(interpolation_config, ",");
    *interpolation = malloc(end_interpolation_config - interpolation_config + 1);
    strncpy(*interpolation, interpolation_config, end_interpolation_config - interpolation_config);
    (*interpolation)[end_interpolation_config - interpolation_config] = '\0';
    
    return 0;
}

int pre_proc_config_data(void *handle, char **align, int *height, int *width, char **interpolation)
{
    // 读取前处理配置数据
    char *pre_proc_config_data;
    int pre_proc_config_size = 0;
    if (0 != GetPreProcConfig(handle, &pre_proc_config_data, &pre_proc_config_size)) {
        printf("GetPreProcessConfig failed\n");
        return -1;
    }

    // 解析pre_proc_config_data，得到resize配置信息
    if (parse_pre_proc_config_data(pre_proc_config_data, align, height, width, interpolation) != 0) {
        printf("parse pre_proc_config_data failed\n");
        return -1;
    }

    // 释放pre_proc_config_data内存
    free(pre_proc_config_data);

    return 0;
}

int main(int argc, char* argv[])
{
    printf("============test sample_runner_pic_c begin============\n");

    void *handle;
    const char *config_file = argv[1];// "/root/gddeploy/install/install_ascend3403/config.json";
    const char *model_path = argv[2];
    const char *license_file = argv[3];
    const char *pic_path = argv[4];
    int img_h = atoi(argv[5]);
    int img_w = atoi(argv[6]);
    int ori_img_w = atoi(argv[7]); 
    int ori_img_h = atoi(argv[8]);;

    printf("model_path: %s\n", model_path);
    printf("license_file: %s\n", license_file);
    printf("pic_path: %s\n", pic_path);
    printf("img_h: %d\n", img_h);
    printf("img_w: %d\n", img_w);

    if ( 0 != Init(&handle, config_file, model_path, license_file)) {
        printf("Init failed\n");
        return -1;
    }

    // 打印前处理配置信息
    char *align;
    int height;
    int width;
    char *interpolation;
    if (pre_proc_config_data(handle, &align, &height, &width, &interpolation) != 0) {
        printf("test pre_proc_config_data failed\n");
        return -1;
    }

    printf("------------pre_proc_config_data------------\n");
    printf("align: %s\n", align);
    printf("height: %d\n", height);
    printf("width: %d\n", width);
    printf("interpolation: %s\n", interpolation);
    printf("----------------------------------------\n");


    // unsigned int input_size = img_h * img_w * 3 * sizeof(float);
    unsigned int input_size = img_h * img_w * 3 / 2 * sizeof(char);
    char *pic_data = (unsigned char*)malloc(input_size);
    if (read_raw_rgb_file(pic_path, pic_data, input_size, img_h, img_w) != 0) {
        printf("read pic file failed\n");
        return -1;
    }

    DetectResult output_result;

    if (0 != Infer(handle, pic_data, input_size, ori_img_w, ori_img_h, img_w, img_h,  &output_result)) {
        printf("Infer failed\n");
        return -1;
    }

    // 打印输出结果    
    printf("--------------result--------------\n");
    for (int i = 0; i < output_result.detect_img_num; i++) {
        DetectImg* detect_img = output_result.detect_imgs + i;
        printf("img_id: %d, img_w: %d, img_h: %d\n", detect_img->img_id, detect_img->img_w, detect_img->img_h);
        for (int j = 0; j < detect_img->num_detect_obj; j++) {
            DetectObject* detect_obj = detect_img->detect_objs + j;
            printf("label: %s, detect_id: %d, class_id: %d, score: %f, x: %f, y: %f, w: %f, h: %f\n", detect_obj->label, detect_obj->detect_id, detect_obj->class_id, detect_obj->score, detect_obj->bbox.x, detect_obj->bbox.y, detect_obj->bbox.w, detect_obj->bbox.h);
        }
    }
    printf("---------------------------------\n");

    free(pic_data);

    if (0 != Deinit(handle)) {
        printf("Release failed\n");
        return -1;
    }

    // release output_result memory
    for (int i = 0; i < output_result.detect_img_num; i++) {
        DetectImg* detect_img = output_result.detect_imgs + i;
        free(detect_img->detect_objs);        
    }
    free(output_result.detect_imgs);

    free(align);
    free(interpolation);

    printf("============test sample_runner_pic_c end============\n");

    return 0;
}