#include <Rtrc/Resource/ResourceManager.h>

RTRC_BEGIN

LocalCachedMaterialHandle::LocalCachedMaterialHandle(
    Ref<ResourceManager> resources, LocalCachedMaterialStorage *storage, std::string_view name)
    : LocalCachedMaterialHandle(resources->GetMaterialManager(), storage, name)
{
    
}

RC<LegacyMaterial> LocalCachedMaterialHandle::Get() const
{
    return materialManager_->GetLocalMaterialCache().Get(storage_, name_);
}

RC<LegacyMaterial> LocalMaterialCache::Get(LocalCachedMaterialStorage *storage, std::string_view materialName)
{
    const uint32_t index = storage->index_;

    {
        std::shared_lock lock(mutex_);
        if(index < materials_.size() && materials_[index])
        {
            return materials_[index];
        }
    }

    std::lock_guard lock(mutex_);
    if(index < materials_.size() && materials_[index])
    {
        return materials_[index];
    }

    if(index >= materials_.size())
    {
        materials_.resize(index + 1);
    }
    if(!materials_[index])
    {
        materials_[index] = materialManager_->GetMaterial(std::string(materialName));
    }
    return materials_[index];
}

RTRC_END
