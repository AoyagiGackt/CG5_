#include "ModelManager.h"

ModelManager* ModelManager::GetInstance()
{
    static ModelManager instance;
    return &instance;
}

void ModelManager::Initialize(ModelCommon* modelCommon)
{
    modelCommon_ = modelCommon;
}

void ModelManager::LoadModel(const std::string& filePath, const std::string& textureFilePath)
{
    if (models_.contains(filePath)) {
        return;
    }

    std::unique_ptr<Model> model = std::make_unique<Model>();
    model->Initialize(modelCommon_, filePath, textureFilePath);

    models_[filePath] = std::move(model);
}

Model* ModelManager::FindModel(const std::string& filePath)
{
    if (models_.contains(filePath)) {
        return models_[filePath].get();
    }

    return nullptr;
}

void ModelManager::Finalize()
{
    models_.clear();
}