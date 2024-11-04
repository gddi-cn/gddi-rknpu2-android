# RK3588 RKNN SDK 推理环境设置和运行示例

本文档提供了在RK3588开发板上设置和运行RKNN SDK推理环境的指导，以及运行示例程序的步骤。

## 环境要求

首先，请确保你的环境满足以下要求：

- 根据 [Rockchip Linux Repository](https://github.com/rockchip-linux/rknpu2.git) 中的指导，确保你的版本为 `v1.4`。
- 遵循《Rockchip_Quick_Start_RKNN_SDK_V1.4.0_CN.pdf》文档中的《3.2.2 更新板子的 rknn_server 和 librknnrt.so》章节，更新推理库。
- 按照《3.3 RKNPU2 的编译及使用方法》中的步骤，确保能够成功运行 `rknn_yolov5_demo` 并确认推理环境正常。

## 设置步骤

### 获取Root权限并重新挂载文件系统

在设置之前，需要获取root权限并重新挂载文件系统：

```sh
adb root
adb remount
```

### 推送SDK到RK3588

使用以下命令将SDK推送到RK3588开发板：

```sh
adb push android_3588_install/ /data/local/tmp/
```

### 设置环境变量

进入adb shell后，设置环境变量：

```sh
adb shell
export LD_LIBRARY_PATH=/data/local/tmp/android_3588_install/thirdparty/android/sdk/native/libs/arm64-v8a:/data/local/tmp/android_3588_install/lib:/data/local/tmp/android_3588_install/thirdparty/rknpu2_1.4.0/lib:/data/local/tmp/android_3588_install/thirdparty/librga_1.10.0/lib:/data/local/tmp/android_3588_install/thirdparty/android/lib
```

### 开启调试打印

设置环境变量以开启调试打印：

```sh
export SPDLOG_LEVEL=trace
```

### 生成授权文件

使用以下命令生成gxt文件并发送给gddi进行授权：

```sh
cd /data/local/tmp/android_3588_install/tools
./gxt_maker
```

这个命令会生成一个 `xxx.gxt` 文件，需要将此文件发送给gddi以获得授权模型和文件。

### 推送模型和授权文件到RK3588

将模型和授权文件推送到RK3588开发板：

```sh
adb push face_eye /data/local/tmp/
```

然后执行示例程序：

```sh
cd /data/local/tmp/android_3588_install/bin
./sample_runner_pic --batch-size 1 --license /data/local/tmp/face_eye/license --model /data/local/tmp/face_eye/model.gdd --pic-path /data/local/tmp/face_eye/0a1cc9c832c569dc578f84537ccb10fd44c58c3c_up.jpg
```

请注意，RK3588只支持 `--batch-size 1` 设置，其他值无效，都会被认为是1。由于rk rga的限制，需要输入图片宽是16对齐，高是2对齐。

## 运行结果

如果运行成功，你将在 `/data/local/tmp/android_3588_install/bin` 目录下看到生成的 `0a1cc9c832c569dc578f84537ccb10fd44c58c3c_up.jpg` 图片。你可以使用 `adb pull` 命令将其拉取到本机查看检测效果。

## 接口说明
int PicRunner::Init(const std::string config, std::vector<std::string> model_paths, std::vector<std::string> license)  
接口描述：初始化接口  
返回值：0 成功，非零 错误。  
config[input]:算法输入参数，目前固定传入string: {"model_param":{"batch_size":1}}  
model_paths[input]:model.gdd路径列表，可以传入多个模型路径，也可以传入一个。  
license[input]:license授权文件路径列表，可以传入多个模型路径，也可以传入一个。 


int PicRunner::InferSync(cv::Mat int_mat,gddeploy::InferResult &result)  
接口描述：同步推理接口，输入一张图片，输出一个结果。这个接口只使用模型列表中的第0个。  
返回值：0 成功，非零 错误。  
int_mat[input]:opencv u8c3 bgr格式图片,由于rk rga的限制，需要输入图片宽是16对齐，高是2对齐。  
result[output]:推理结果，不同算法类型结果不同，检测结果定义：  

### 检测结果定义

检测结果的定义位于 `{SDK_PATH}/result_def.h`：

```cpp
typedef struct {
  // int label;
  int detect_id;        
  int class_id; //类别         
  float score;  //置信度  
  Bbox bbox;    //检测框位置        
  std::string label;    //标签名称
}DetectObject;  // struct DetectObject
```
### 获取结果的方法参考
```cpp
// class DetectResult : public InferResult{
typedef struct{
    uint32_t batch_size = 0;
    std::vector<DetectImg> detect_imgs;
    
    void PrintResult() { 
        std::cout << "Detect result: " << std::endl; 
        for (auto detect_img : detect_imgs) {
            std::cout << "Img id: " << detect_img.img_id << std::endl; 
            for (auto obj : detect_img.detect_objs) {
                std::cout << "detect id: " << obj.detect_id << " name: " << obj.label
                    << " class id: " << obj.class_id  << " score: " << obj.score
                    << " box[" << obj.bbox.x << ", " << obj.bbox.y << ", " << obj.bbox.w << ", " << obj.bbox.h << "]" << std::endl;
            }
        }
    }
}DetectResult;

```

### 正脸检测改造

为了改造成正脸检测，需要根据场景加入以下逻辑：

1. 设置检测框的宽度大于某个阈值。
2. 确保人脸框中包含两个人眼框（可以使用IOU来判断）。
3. 计算左眼框中心x和右眼框中心x到人脸检测框中心x的比值，以判断侧脸的角度，如果角度在一定阈值内则认为是正脸。


### 编译
为了减少编译问题，提供编译docker()

1. 使用该镜像仓库前，需设置 Docker 安全域名：
    - 根据需要，将域名 gddi2020.qicp.vip:12580 配置到 Docker Daemon insecure-registries 中，一般在/etc/docker/daemon.json
    ```json
    {                                                                             
        "insecure-registries":[
            "http://gddi2020.qicp.vip:12580"
        ],
        ...
    }
    ```
2. 拉取docker: 
    ```
    docker pull gddi2020.qicp.vip:12580/rk/gddeploy_compile_rknpu2_android:v0.0.1
    ```
3. 使用镜像启动容器。
    ```sh
    docker run -it --privileged=true gddi2020.qicp.vip:12580/rk/gddeploy_compile_rknpu2_android:v0.0.1 /bin/bash
    ```
4. 镜像中已经放了ndk编译器，路径：/opt/android-ndk-r17c
5. 使用docker cp 或则nfs挂载等方式，将sdk拷贝入docker中，例如拷贝到：/volume1/code3/android_3588_install
6. 编译命令
    ```shell
    cd /volume1/code3/android_3588_install 
    mkdir -p ./build && cd build 
    cmake -DBUILD_SYSTEM_NAME=android -DBUILD_TARGET_CHIP=rk3588 --install-prefix /volume1/code3/android_3588_install/install .. 
    make -j8 
    make install
    ```
    编译完成后，会被安装到/volume1/code3/android_3588_install/install


