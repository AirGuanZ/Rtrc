#pragma once

#include <atomic>
#include <vector>

#include <tbb/spin_rw_mutex.h>

#include <Core/SmartPointer/ObserverPtr.h>
#include <Core/Uncopyable.h>
#include <Graphics/Shader/ShaderDatabase.h>

RTRC_BEGIN

class LegacyMaterial;
class LegacyMaterialManager;
class ResourceManager;
class Shader;
class ShaderManager;

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

class LocalShaderCache : public Uncopyable
{
public:

    explicit LocalShaderCache(Ref<ShaderManager> shaderManager)
        : shaderManager_(shaderManager)
    {

    }

    const RC<ShaderTemplate> &Get(LocalCachedShaderStorage *storage, std::string_view materialName);

private:

    Ref<ShaderManager> shaderManager_;
    tbb::spin_rw_mutex mutex_;
    std::vector<RC<ShaderTemplate>> shaders_;
};

RTRC_END
