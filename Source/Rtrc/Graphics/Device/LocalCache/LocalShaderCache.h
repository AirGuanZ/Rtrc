#pragma once

#include <atomic>
#include <vector>

#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Utility/SmartPointer/ObserverPtr.h>
#include <Rtrc/Utility/Uncopyable.h>

RTRC_BEGIN

class BuiltinResourceManager;
class Material;
class MaterialManager;
class Shader;
class ShaderTemplate;

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
        ObserverPtr<BuiltinResourceManager> builtinResources,
        LocalCachedShaderStorage           *storage,
        std::string_view                    name);
    
    operator RC<ShaderTemplate>() const
    {
        return Get();
    }

    ShaderTemplate *operator->() const
    {
        return Get().get();
    }

    RC<ShaderTemplate> Get() const;

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

    RC<ShaderTemplate> Get(LocalCachedShaderStorage *storage, std::string_view materialName);

private:

    ObserverPtr<MaterialManager> materialManager_;
    tbb::spin_rw_mutex mutex_;
    std::vector<RC<ShaderTemplate>> shaders_;
};

#define RTRC_STATIC_SHADER_TEMPLATE(MANAGER, HANDLE_NAME, NAME) \
    RTRC_STATIC_SHADER_TEMPLATE_IMPL(MANAGER, HANDLE_NAME, NAME)
#define RTRC_STATIC_SHADER_TEMPLATE_IMPL(MANAGER, HANDLE_NAME, NAME)           \
    static ::Rtrc::LocalCachedShaderStorage _staticShaderStorage##HANDLE_NAME; \
    ::Rtrc::LocalCachedShaderHandle HANDLE_NAME{ (MANAGER), &_staticShaderStorage##HANDLE_NAME, NAME }

#define RTRC_GET_CACHED_SHADER_TEMPLATE(MANAGER, NAME)             \
    [&]                                                            \
    {                                                              \
        RTRC_STATIC_SHADER_TEMPLATE((MANAGER), _tempShader, NAME); \
        return _tempShader.Get();                                  \
    }()

#define RTRC_GET_CACHED_SHADER(MANAGER, NAME)                      \
    [&]                                                            \
    {                                                              \
        RTRC_STATIC_SHADER_TEMPLATE((MANAGER), _tempShader, NAME); \
        return _tempShader.Get();                                  \
    }()->GetShader()

RTRC_END
