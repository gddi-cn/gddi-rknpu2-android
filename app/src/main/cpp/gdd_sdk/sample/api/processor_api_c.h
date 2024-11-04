#ifndef __PROCESSOR_API_C_H__
#define __PROCESSOR_API_C_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "capi/result_def_c.h"
#include "core/mem/buf_surface.h"

int Init(void **handle, const char *config_file, const char *model_path, const char *license_file);

int Deinit(void *handle);

int GetLabels(void *handle, char **output_labels, int *output_label_size);

int GetPreProcConfig(void *handle, char **output_config, int *output_config_size);

int Infer(void *handle, const char *input_data, int input_size, int ori_img_w, int ori_img_h, int img_w, int img_h,
          DetectResult *output_result);

int InitJna(void **handle, const char *config, const char *model_path, const char *license_file);
int InferJna(void *handle, const char *input_data, int input_size, BufSurfaceColorFormat format, int img_w, int img_h,
             DetectResult *output_result);
#ifdef __cplusplus
}
#endif

#endif /* __PROCESSOR_API_C_H__ */