#pragma once

#include <atomic>
#include <vector>

#include <tbb/spin_rw_mutex.h>

#include <Core/SmartPointer/ObserverPtr.h>
#include <Core/Uncopyable.h>

RTRC_BEGIN

class LegacyMaterial;
class LegacyMaterialManager;
class ResourceManager;

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
        ObserverPtr<LegacyMaterialManager> materialManager,
        LocalCachedMaterialStorage  *storage,
        std::string_view             name)
        : materialManager_(materialManager), storage_(storage), name_(name)
    {

    }
    
    LocalCachedMaterialHandle(
        ObserverPtr<ResourceManager> resources,
        LocalCachedMaterialStorage  *storage,
        std::string_view             name);
    
    operator RC<LegacyMaterial>() const
    {
        return Get();
    }

    LegacyMaterial* operator->() const
    {
        return Get().get();
    }

    RC<LegacyMaterial> Get() const;

private:

    ObserverPtr<LegacyMaterialManager> materialManager_;
    LocalCachedMaterialStorage  *storage_;
    std::string_view             name_;
};

class LocalMaterialCache : public Uncopyable
{
public:

    explicit LocalMaterialCache(ObserverPtr<LegacyMaterialManager> materialManager)
        : materialManager_(materialManager)
    {
        
    }

    RC<LegacyMaterial> Get(LocalCachedMaterialStorage *storage, std::string_view materialName);

private:

    ObserverPtr<LegacyMaterialManager> materialManager_;
    tbb::spin_rw_mutex mutex_;
    std::vector<RC<LegacyMaterial>> materials_;
};

RTRC_END
