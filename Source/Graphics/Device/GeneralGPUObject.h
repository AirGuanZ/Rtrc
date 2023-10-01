#pragma once

#include <Graphics/Device/DeviceSynchronizer.h>

RTRC_BEGIN

class GeneralGPUObjectManager;

template<typename RHIObjectPtr>
class GeneralGPUObject : public Uncopyable
{
public:

    ~GeneralGPUObject();

    const RHIObjectPtr &GetRHIObject() const;

    void SetName(std::string name);

protected:

    friend class GeneralGPUObjectManager;

    GeneralGPUObjectManager *manager_ = nullptr;
    RHIObjectPtr rhiObject_;
};

// Simple delayed rhi object releasing
class GeneralGPUObjectManager
{
public:

    explicit GeneralGPUObjectManager(DeviceSynchronizer &sync);
    
    template<typename RHIObjectPtr>
    void _internalRelease(GeneralGPUObject<RHIObjectPtr> &object);

protected:
    
    DeviceSynchronizer &sync_;
};

template<typename RHIObjectPtr>
GeneralGPUObject<RHIObjectPtr>::~GeneralGPUObject()
{
    if(manager_)
    {
        manager_->_internalRelease(*this);
    }
}

template<typename RHIObjectPtr>
const RHIObjectPtr &GeneralGPUObject<RHIObjectPtr>::GetRHIObject() const
{
    return rhiObject_;
}

template<typename RHIObjectPtr>
void GeneralGPUObject<RHIObjectPtr>::SetName(std::string name)
{
    rhiObject_->SetName(std::move(name));
}

inline GeneralGPUObjectManager::GeneralGPUObjectManager(DeviceSynchronizer &sync)
    : sync_(sync)
{

}

template<typename RHIObjectPtr>
void GeneralGPUObjectManager::_internalRelease(GeneralGPUObject<RHIObjectPtr> &object)
{
    sync_.OnFrameComplete([o = std::move(object.rhiObject_)] {});
}

RTRC_END
