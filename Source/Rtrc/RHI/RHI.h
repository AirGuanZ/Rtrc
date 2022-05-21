#pragma once

#include <vector>

#include <Rtrc/Utils/Uncopyable.h>
#include <Rtrc/Window/Window.h>

RTRC_RHI_BEGIN

// =============================== rhi enums ===============================

enum class QueueType : uint8_t
{
    Graphics,
    Compute,
    Transfer
};

enum class TexelFormat : uint32_t
{
    B8G8R8A8_UNorm
};

const char *GetTexelFormatName(TexelFormat format);

// =============================== rhi descriptions ===============================

struct DeviceDescription
{
    bool graphicsQueue    = true;
    bool computeQueue     = false;
    bool transferQueue    = false;
    bool supportSwapchain = true;
};

struct SwapchainDescription
{
    TexelFormat format;
};

// =============================== rhi interfaces ===============================

class Instance;
class Device;
class Queue;
class Swapchain;
class Texture;
class Texture2D;
class Texture2DSRV;
class Texture2DUAV;
class Texture2DRTV;
class Texture2DDSV;
class Buffer;
class BufferSRV;
class BufferUAV;

class RHIObject : public Uncopyable
{
protected:

    std::string RHIObjectName_;

public:

    virtual ~RHIObject() = default;

    virtual void SetName(std::string name) { RHIObjectName_ = std::move(name); }

    virtual const std::string &GetName() { return RHIObjectName_; }
};

class Surface : public RHIObject
{
    
};

class Instance : public RHIObject
{
public:

    virtual Unique<Device> CreateDevice(const DeviceDescription &desc = {}) = 0;
};

class Device : public RHIObject
{
public:

    virtual Unique<Queue> GetQueue(QueueType type) = 0;

    virtual Unique<Swapchain> CreateSwapchain(const SwapchainDescription &desc, Window &window) = 0;
};

class Queue : public RHIObject
{
public:

    virtual bool IsInSameFamily(const Queue &other) const = 0;
};

class Swapchain : public RHIObject
{
public:

    virtual Shared<Texture2D> GetCurrentBackTexture() const = 0;
};

// =============================== vulkan backend ===============================

// can be called multiple times
void InitializeVulkanBackend();

#ifdef RTRC_RHI_VULKAN

struct VulkanInstanceDescription
{
    std::vector<std::string> extensions;
    bool debugMode = false;
};

Unique<Instance> CreateVulkanInstance(const VulkanInstanceDescription &desc);

#endif

RTRC_RHI_END
