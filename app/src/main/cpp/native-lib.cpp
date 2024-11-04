#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_gddandroid_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject) {
    std::string hello = "Hello from JNI C++";
    return env->NewStringUTF(hello.c_str());
}