#include <Rtrc/Resource/ResourceManager.h>
#include <Rtrc/Resource/ShaderManager.h>

RTRC_BEGIN

const RC<ShaderTemplate> &LocalShaderCache::Get(LocalCachedShaderStorage *storage, std::string_view materialName)
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
        shaders_[index] = shaderManager_->GetShaderTemplate(materialName, true);
    }
    return shaders_[index];
}

RTRC_END
