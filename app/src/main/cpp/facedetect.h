#include <jni.h>
#include <string>

#include "capi/processor_api_c.h"
#include <dlfcn.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "core/result_def.h"
#include "app/runner_pic.h"

extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_gddandroid_MainActivity_GddInit(JNIEnv *env, jobject thiz, jstring config_file, jstring model_path,
                                                 jstring license_file, jstring pic_path);


extern "C"
JNIEXPORT jint JNICALL
Java_com_example_gddandroid_MainActivity_GddDeInit(JNIEnv *env, jobject thiz, jlong handle);