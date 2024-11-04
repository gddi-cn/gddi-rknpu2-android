#include <jni.h>
#include <string>
#include "capi/processor_api_c.h"
#include <dlfcn.h>
#include <android/log.h>

#include "core/result_def.h"
#include "app/runner_pic.h"

extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_gddandroid_MainActivity_GddInit(JNIEnv *env, jobject thiz, jstring config,
                                                 jstring model_path,
                                                 jstring licence, jstring picPath) {
    void *handle;
    const char *nativeConfigFile = env->GetStringUTFChars(config, nullptr);
    const char *nativeModelPath = env->GetStringUTFChars(model_path, nullptr);
    const char *nativeLicenseFile = env->GetStringUTFChars(licence, nullptr);
    const char *nativepicFile = env->GetStringUTFChars(picPath, nullptr);

    cv::Mat img = cv::imread(nativepicFile);

    // 1.方法1
    //  int result = Init(&handle, "{\"model_param\":{\"batch_size\":1}}", nativeModelPath, nativeLicenseFile);
    // 2.方法2
    int result = InitJna(&handle, "{\"model_param\":{\"batch_size\":4}}", nativeModelPath,
                         nativeLicenseFile);
    
    __android_log_write(ANDROID_LOG_INFO, "uuu", "This is a C++ log message!!!!!!!!!!!!!!!!!!!");

    env->ReleaseStringUTFChars(config, nativeConfigFile);
    env->ReleaseStringUTFChars(model_path, nativeModelPath);
    env->ReleaseStringUTFChars(licence, nativeLicenseFile);
    env->ReleaseStringUTFChars(picPath, nativepicFile);
    if (result == 0) {
        // 测试推理
        DetectResult result;

        int inferRes1 = InferJna(handle, (char *) img.data, img.cols * img.rows * img.channels(),
                                 GDDEPLOY_BUF_COLOR_FORMAT_BGR,
                                 img.cols, img.rows, &result);

        return reinterpret_cast<jlong>(handle);
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_gddandroid_MainActivity_GddDeInit(JNIEnv *env, jobject thiz, jlong handle) {
    int res = Deinit((void *) handle);
    return res;
}

