#include <fstream>
#include <iostream>

#include "api/global_config.h"
#include "core/pipeline.h"
#include "core/mem/buf_surface.h"
#include "core/processor.h"
#include "core/mem/buf_surface_util.h"

#include "api/processor_api.h"
#include "core/result_def.h"

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

int main(int argc, char **argv)
{
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

    // 获取procesor，只有推理单元
    gddeploy::gddeploy_init("");

    std::string config = "skip_preproc";
    gddeploy::ProcessorAPI api;
    api.Init(config, model_path);


    auto processors = api.GetProcessor();

    // 准备和解析数据
    unsigned int input_size = img_h * img_w * 3 / 2 * sizeof(char);
    char *pic_data = (unsigned char*)malloc(input_size);
    if (read_raw_rgb_file(pic_path, pic_data, input_size, img_h, img_w) != 0) {
        printf("read pic file failed\n");
        return -1;
    }

    BufSurface *surface = new BufSurface();
    surface->mem_type = GDDEPLOY_BUF_MEM_SYSTEM;
    surface->batch_size = 1;
    surface->num_filled = 1;

    BufSurfacePlaneParams plane_param;
    memset(&plane_param, 0, sizeof(BufSurfacePlaneParams));
    plane_param.num_planes = 1;
    plane_param.width[0] = img_w;
    plane_param.height[0] = img_h;

    BufSurfaceParams *param = new BufSurfaceParams();
    param->plane_params = plane_param;
    param->color_format = GDDEPLOY_BUF_COLOR_FORMAT_BGR;
    param->data_size = img_w * img_h * 3 * sizeof(uint8_t);
    param->width = img_w;
    param->height = img_h;
    param->data_ptr = pic_data;
    
    surface->surface_list = param;
    gddeploy::BufSurfWrapperPtr surf = std::make_shared<gddeploy::BufSurfaceWrapper>(surface, false);

    gddeploy::PackagePtr in = gddeploy::Package::Create(1);
    in->predict_io = std::make_shared<gddeploy::InferData>();
    in->predict_io->Set(std::move(surf));

    // 推理
    for (auto processor : processors){
        processor->Process(in);
    }

    // 取出数据和保存
    // std::vector<BufSurfWrapperPtr> out_bufs = pack->predict_io->GetLref<std::vector<BufSurfWrapperPtr>>();
    
    const gddeploy::InferResult& postproc_results = in->data[0]->GetLref<gddeploy::InferResult>();
     for (auto result_type : postproc_results.result_type){
        if (result_type == gddeploy::GDD_RESULT_TYPE_DETECT){

            for (auto &obj : postproc_results.detect_result.detect_imgs[0].detect_objs) {

                std::cout << "detect result: " << "box[" << obj.bbox.x \
                        << ", " << obj.bbox.y << ", " << obj.bbox.w << ", " \
                        << obj.bbox.h << "]" \
                        << "   score: " << obj.score << std::endl;
            }
        }
     }

    // const std::vector<SegImg>& seg_imgs = postproc_results.seg_result.seg_imgs;
    // std::ofstream ofile(pic_name);
    // if(ofile.is_open()==false){
    //     std::cout << strerror(errno) << std::endl;
    //     continue;
    // }

    // for (auto &surf : out_bufs){
    //     auto src_param = surf->GetSurfaceParams();
    //     ofile.write((char*)surf->GetData(0, 0), src_param->data_size);
    // }
    
    // ofile.close();

    delete surface;
    delete param;

    return 0;
}


