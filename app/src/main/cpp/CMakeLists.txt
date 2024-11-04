# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.0.1)
project("gddandroid")

include_directories(${CMAKE_SOURCE_DIR}/gdd_sdk/include)
link_directories(${CMAKE_SOURCE_DIR}/../lib/${ANDROID_ABI})

#set(OpenCV_DIR ${CMAKE_PROJECT_NAME}/sdk/native/jni)
#find_package(OpenCV REQUIRED)

#set(ANDROID_OPENCV_COMPONENTS "opencv_java" CACHE STRING "")
#message(STATUS "ANDROID_ABI=${ANDROID_ABI}")
#find_package(OpenCV REQUIRED COMPONENTS ${ANDROID_OPENCV_COMPONENTS})


#link_directories(${CMAKE_SOURCE_DIR}/lib/${ANDROID_ABI})
#include_directories(${JNI_SRC_DIR}/demo)
#
#file(GLOB CMakeFiles "${JNI_SRC_DIR}/gdd_sdk/cmake/*.cmake")
#foreach (cmake_module_file IN LISTS CMakeFiles)
#    # get_filename_component(CMakeName ${cmake_module_file} NAME_WLE)
#    get_filename_component(CMakePath ${cmake_module_file} DIRECTORY)
#    message("LoadCMake: ${CMakeName}")
#    include(${cmake_module_file})
#endforeach (cmake_module_file)

add_library(${CMAKE_PROJECT_NAME} SHARED
        facedetect.cpp
        native-lib.cpp)

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        gddeploy_capi
        gddeploy_app
        #        ${CMAKE_PROJECT_NAME}/app/src/main/cpp/sdk/native//libs/${ANDROID_ABI}/libopencv_java4.so
        ${CMAKE_SOURCE_DIR}/sdk/native//libs/${ANDROID_ABI}/libopencv_java4.so
        #    ${CMAKE_SOURCE_DIR}/lib/${ANDROID_ABI}/libgddeploy_capi.so.1.0.3
        #    ${CMAKE_SOURCE_DIR}/lib/${ANDROID_ABI}/libgddeploy_core
        #    ${CMAKE_SOURCE_DIR}/lib/${ANDROID_ABI}/libgddeploy_register
        #    ${CMAKE_SOURCE_DIR}/lib/${ANDROID_ABI}/libgddeploy_common.so.
        log)
