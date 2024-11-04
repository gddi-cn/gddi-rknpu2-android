#include <vector>
#include <memory>
#include "core/processor.h"
#include "core/picture.h"

namespace gddeploy{

class ProcessorAPIPriv;
//api的接口根据解析配置文件和模型文件，获取前中后处理单元
class ProcessorAPI{
public:
    ProcessorAPI();
    int Init(std::string config, std::string model_path, std::string license = "");

    // 根据模型获取processor，用于最基础层的接口
    std::vector<ProcessorPtr> GetProcessor();

    std::string GetModelType();
    std::vector<std::string> GetLabels();

    std::string GetPreProcConfig();

    Picture* GetPicture();
    bool IsSupportHwPicDecode();

private:
    std::shared_ptr<ProcessorAPIPriv> priv_;
};

}