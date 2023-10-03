#pragma once

#include <atomic>
#include <vector>

#include <tbb/spin_rw_mutex.h>

#include <Core/SmartPointer/ObserverPtr.h>
#include <Core/Uncopyable.h>
#include <Graphics/Shader/ShaderDatabase.h>

RTRC_BEGIN

class Material;
class MaterialManager;
class ResourceManager;
class Shader;

class LocalCachedShaderStorage : public Uncopyable
{
public:

    LocalCachedShaderStorage()
    {
        static std::atomic<uint32_t> nextIndex = 0;
        index_ = nextIndex++;
    }

private:

    friend class LocalShaderCache;

    uint32_t index_;
};

class LocalCachedShaderHandle
{
public:

    LocalCachedShaderHandle(
        ObserverPtr<MaterialManager> materialManager,
        LocalCachedShaderStorage    *storage,
        std::string_view             name)
        : materialManager_(materialManager), storage_(storage), name_(name)
    {

    }
    
    LocalCachedShaderHandle(
        ObserverPtr<ResourceManager> resources,
        LocalCachedShaderStorage    *storage,
        std::string_view             name);
    
    operator const RC<ShaderTemplate>&() const
    {
        return Get();
    }

    ShaderTemplate *operator->() const
    {
        return Get().get();
    }

    const RC<ShaderTemplate> &Get() const;

private:

    ObserverPtr<MaterialManager> materialManager_;
    LocalCachedShaderStorage *storage_;
    std::string_view name_;
};

class LocalShaderCache : public Uncopyable
{
public:

    explicit LocalShaderCache(ObserverPtr<MaterialManager> materialManager)
        : materialManager_(materialManager)
    {
        
    }

    const RC<ShaderTemplate> &Get(LocalCachedShaderStorage *storage, std::string_view materialName);

private:

    ObserverPtr<MaterialManager> materialManager_;
    tbb::spin_rw_mutex mutex_;
    std::vector<RC<ShaderTemplate>> shaders_;
};

RTRC_END
