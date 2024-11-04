#include <memory>

#include "core/model.h"
#include "api/processor_api.h"
#include "core/pipeline.h"
#include "core/register.h"
#include "core/picture.h"
#include "core/alg.h"
#include "common/logger.h"

using namespace gddeploy;

namespace gddeploy{

//api的接口根据解析配置文件和模型文件，获取前中后处理单元
class ProcessorAPIPriv{
public:
    ProcessorAPIPriv(){};

    ~ProcessorAPIPriv(){
        ModelManager::Instance()->Unload(model_);
    }
    int Init(std::string config, std::string model_path, std::string license = "");

    // 根据模型获取processor，用于最基础层的接口
    std::vector<ProcessorPtr> GetProcessor(){
        return processors_;
    }

    // 用于添加更多的processor再后面，再设置进去
    int SetProcessor(std::vector<ProcessorPtr> processors);

    std::string GetModelType();
    std::vector<std::string> GetLabels();

    Picture* GetPicture(){ return picture_; }
    bool IsSupportHwPicDecode(){ return picture_ != nullptr; }

    std::string GetPreProcConfig();

private:
    std::vector<ProcessorPtr> processors_;

    gddeploy::Picture* picture_;
    ModelPtr model_;
};
} 

int ProcessorAPIPriv::Init(std::string config, std::string model_path, std::string license)
{
    //载入model
    std::string properties_path = "";
    model_ = gddeploy::ModelManager::Instance()->Load(model_path, properties_path, license, config);
    if (model_ == nullptr){
        GDDEPLOY_ERROR("[app] load model false");
        return -1;
    }

    // 创建picture
    auto properties = model_->GetModelInfoPriv();
    std::string manufacturer = properties->GetProductType();
    std::string chip = properties->GetChipType();
    picture_ = Picture::Instance()->GetPicture(manufacturer, chip);

    // 创建processors
    Pipeline::Instance()->CreatePipeline(config, model_, processors_);

    return 0;
}

int ProcessorAPIPriv::SetProcessor(std::vector<ProcessorPtr> processors){
    // for (auto processor : processors){
    //     processors_.emplace_back(processor);
    // }
    return 0;
}

std::string ProcessorAPIPriv::GetModelType()
{
    auto model_info_priv = model_->GetModelInfoPriv();
    return model_info_priv->GetModelType();
}

std::vector<std::string> ProcessorAPIPriv::GetLabels()
{
    ModelPropertiesPtr mp = model_->GetModelInfoPriv();
    
    return mp->GetLabels();
}

std::string ProcessorAPIPriv::GetPreProcConfig()
{
    auto model_info_priv = model_->GetModelInfoPriv();
    std::string model_type = model_info_priv->GetModelType();
    std::string net_type = model_info_priv->GetNetType();

    auto alg_creator = AlgManager::Instance()->GetAlgCreator(model_type, net_type);
    AlgPtr alg_ptr = alg_creator->Create();

    alg_ptr->Init("", model_);   
    
    return alg_ptr->GetPreProcConfig();
}

ProcessorAPI::ProcessorAPI(){
    priv_ = std::make_shared<ProcessorAPIPriv>();
}

int ProcessorAPI::Init(std::string config, std::string model_path, std::string license)
{
    return priv_->Init(config, model_path, license);
}

std::vector<ProcessorPtr> ProcessorAPI::GetProcessor()
{
    return priv_->GetProcessor();
}

std::string ProcessorAPI::GetModelType()
{
    return priv_->GetModelType();
}

std::vector<std::string> ProcessorAPI::GetLabels()
{
    return priv_->GetLabels();
}

std::string ProcessorAPI::GetPreProcConfig()
{
    return priv_->GetPreProcConfig();
}

Picture* ProcessorAPI::GetPicture()
{
    return priv_->GetPicture();
}

bool ProcessorAPI::IsSupportHwPicDecode()
{
    return priv_->IsSupportHwPicDecode();
}