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

class LocalCachedMaterialStorage : public Uncopyable
{
public:

    LocalCachedMaterialStorage()
    {
        static std::atomic<uint32_t> nextIndex = 0;
        index_ = nextIndex++;
    }

private:

    friend class LocalMaterialCache;

    uint32_t index_;
};

class LocalCachedMaterialHandle
{
public:

    LocalCachedMaterialHandle(
        ObserverPtr<MaterialManager> materialManager,
        LocalCachedMaterialStorage  *storage,
        std::string_view             name)
        : materialManager_(materialManager), storage_(storage), name_(name)
    {

    }
    
    LocalCachedMaterialHandle(
        ObserverPtr<BuiltinResourceManager> builtinResources,
        LocalCachedMaterialStorage         *storage,
        std::string_view                    name);
    
    operator RC<Material>() const
    {
        return Get();
    }

    Material* operator->() const
    {
        return Get().get();
    }

    RC<Material> Get() const;

private:

    ObserverPtr<MaterialManager> materialManager_;
    LocalCachedMaterialStorage *storage_;
    std::string_view name_;
};

class LocalMaterialCache : public Uncopyable
{
public:

    explicit LocalMaterialCache(ObserverPtr<MaterialManager> materialManager)
        : materialManager_(materialManager)
    {
        
    }

    RC<Material> Get(LocalCachedMaterialStorage *storage, std::string_view materialName);

private:

    ObserverPtr<MaterialManager> materialManager_;
    tbb::spin_rw_mutex mutex_;
    std::vector<RC<Material>> materials_;
};

#define RTRC_STATIC_MATERIAL(MANAGER, HANDLE_NAME, NAME) \
    RTRC_STATIC_MATERIAL_IMPL(MANAGER, HANDLE_NAME, NAME)
#define RTRC_STATIC_MATERIAL_IMPL(MANAGER, HANDLE_NAME, NAME)                      \
    static ::Rtrc::LocalCachedMaterialStorage _staticMaterialStorage##HANDLE_NAME; \
    ::Rtrc::LocalCachedMaterialHandle HANDLE_NAME{ (MANAGER), &_staticMaterialStorage##HANDLE_NAME, NAME }

#define RTRC_GET_CACHED_MATERIAL(MANAGER, NAME)               \
    [&]                                                       \
    {                                                         \
        RTRC_STATIC_MATERIAL((MANAGER), _tempMaterial, NAME); \
        return _tempMaterial.Get();                           \
    }()

RTRC_END
