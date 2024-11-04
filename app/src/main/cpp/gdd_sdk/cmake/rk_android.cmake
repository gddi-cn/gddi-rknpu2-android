if (BUILD_TARGET_CHIP MATCHES "rk" AND BUILD_SYSTEM_NAME MATCHES "android")
    message("Build for rk on android.")
    set(NDK_VERSION r17c)
    set(API_LEVEL 24)
    set(TARGET_ARCH arm64-v8a)
    set(TOOLSET clang)
    set(PREFIX /usr/local/android)
    set(ANDROID_NDK /opt/android-ndk-r17c)
    set(SYSROOT ${ANDROID_NDK}/platforms/android-${API_LEVEL}/arch-arm64)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --target=aarch64-none-linux-android --gcc-toolchain=${ANDROID_NDK}/toolchains/aarch64-linux-android-4.9/prebuilt/linux-x86_64 -fpic --sysroot ${SYSROOT} -isystem ${ANDROID_NDK}/sources/cxx-stl/llvm-libc++abi/include -isystem ${PREFIX}/include -isystem ${ANDROID_NDK}/sources/cxx-stl/llvm-libc++/include -isystem ${ANDROID_NDK}/sysroot/usr/include -isystem ${ANDROID_NDK}/sysroot/usr/include/aarch64-linux-android -D__ANDROID_API__=${API_LEVEL} -std=c++17 -fPIE -fopenmp -O1 -DTIC_EXPORT=0 -Werror=return-type") 

    add_compile_definitions(WITH_RK)
    # link_directories(/opt/gcc-buildroot-9.3.0-2020.03-x86_64_aarch64-rockchip-linux-gnu-firefly/aarch64-rockchip-linux-gnu/lib64)

    include_directories(${CMAKE_SOURCE_DIR}/thirdparty/android/include/)
    link_directories(${CMAKE_SOURCE_DIR}/thirdparty/android/lib/)

    include_directories(${CMAKE_SOURCE_DIR}/thirdparty/android/sdk/native/jni/include)
    link_directories(${CMAKE_SOURCE_DIR}/thirdparty/android/sdk/native/libs/arm64-v8a/)

    include_directories(${CMAKE_SOURCE_DIR}/thirdparty/rknpu2_1.4.0/include/)
    link_directories(${CMAKE_SOURCE_DIR}/thirdparty/rknpu2_1.4.0/lib/)

    include_directories(${CMAKE_SOURCE_DIR}/thirdparty/librga_1.10.0/include/)
    link_directories(${CMAKE_SOURCE_DIR}/thirdparty/librga_1.10.0/lib/)

    set(RK_MPP_PATH ${CMAKE_SOURCE_DIR}/thirdparty/rk_android_mpp)
    include_directories("${RK_MPP_PATH}/include/")
    include_directories("${RK_MPP_PATH}/include/rockchip/")
    link_directories("${RK_MPP_PATH}/lib")
    
    set(COMMON_LIB rknnrt rga dl c++_shared)
elseif()
    message("Unsupported build target chip: ${BUILD_TARGET_CHIP} BUILD_SYSTEM_NAME {BUILD_SYSTEM_NAME}")
endif()