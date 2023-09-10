#include <Rtrc/Graphics/Device/LocalCache/LocalShaderCache.h>
#include <Rtrc/Resource/ResourceManager.h>

RTRC_BEGIN

LocalCachedShaderHandle::LocalCachedShaderHandle(
    ObserverPtr<ResourceManager> resources, LocalCachedShaderStorage *storage, std::string_view name)
    : LocalCachedShaderHandle(resources->GetMaterialManager(), storage, name)
{
    
}

RC<ShaderTemplate> LocalCachedShaderHandle::Get() const
{
    return materialManager_->GetLocalShaderCache().Get(storage_, name_);
}

RC<ShaderTemplate> LocalShaderCache::Get(LocalCachedShaderStorage *storage, std::string_view materialName)
{
    const uint32_t index = storage->index_;

    {
        std::shared_lock lock(mutex_);
        if(index < shaders_.size() && shaders_[index])
        {
            return shaders_[index];
        }
    }

    std::lock_guard lock(mutex_);
    if(index < shaders_.size() && shaders_[index])
    {
        return shaders_[index];
    }

    if(index >= shaders_.size())
    {
        shaders_.resize(index + 1);
    }
    if(!shaders_[index])
    {
        shaders_[index] = materialManager_->GetShaderTemplate(std::string(materialName));
    }
    return shaders_[index];
}

RTRC_END
