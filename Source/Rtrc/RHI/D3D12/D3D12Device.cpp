#include <Rtrc/RHI/D3D12/D3D12Buffer.h>
#include <Rtrc/RHI/D3D12/D3D12Device.h>
#include <Rtrc/RHI/D3D12/D3D12Queue.h>
#include <Rtrc/RHI/D3D12/D3D12Texture.h>
#include <Rtrc/RHI/Window.h>

#include <dxgi1_4.h>
#include <dxgidebug.h>

RTRC_RHI_D3D12_BEGIN

static void EnableD3D12Debug(bool gpuValidation)
{
    ComPtr<ID3D12Debug> debugController;
    if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
    {
        debugController->EnableDebugLayer();
        if(gpuValidation)
        {
            ComPtr<ID3D12Debug1> debug1;
            RTRC_D3D12_FAIL_MSG(
                debugController->QueryInterface(IID_PPV_ARGS(debug1.GetAddressOf())),
                "failed to quert debug controller interface");
            debug1->SetEnableGPUBasedValidation(TRUE);
        }
    }
    else
    {
        OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
    }

    ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
    if(SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
    {
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
    }
}

static ReferenceCountedPtr<D3D12Queue> CreateD3D12Queue(ID3D12Device *device, const D3D12_COMMAND_QUEUE_DESC &desc, QueueType type)
{
    ComPtr<ID3D12CommandQueue> queue;
    RTRC_D3D12_FAIL_MSG(
        device->CreateCommandQueue(&desc, IID_PPV_ARGS(queue.GetAddressOf())),
        "Failed to create d3d12 queue");
    return MakeReferenceCountedPtr<D3D12Queue>(device, std::move(queue), type);
}

ReferenceCountedPtr<D3D12Device> D3D12Device::Create(const DeviceDesc &desc)
{
    auto ret = ReferenceCountedPtr(new D3D12Device);

    // Device

    if(desc.debugMode)
    {
        EnableD3D12Debug(desc.gpuValidation);
    }

    ComPtr<IDXGIFactory4> factory;
    RTRC_D3D12_FAIL_MSG(
        CreateDXGIFactory(IID_PPV_ARGS(factory.GetAddressOf())),
        "Failed to create dxgi factory");

    ComPtr<IDXGIAdapter> adapter;
    RTRC_D3D12_FAIL_MSG(
        factory->EnumAdapters(0, adapter.GetAddressOf()),
        "Failed to get dxgi adapter");

    ComPtr<ID3D12Device5> device;
    RTRC_D3D12_FAIL_MSG(
        D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(device.GetAddressOf())),
        "Failed to create directx12 device");

    // Queues

    D3D12_COMMAND_QUEUE_DESC queueDesc;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    ReferenceCountedPtr<D3D12Queue> graphicsQueue;
    if(desc.graphicsQueue)
    {
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        graphicsQueue = CreateD3D12Queue(device.Get(), queueDesc, QueueType::Graphics);
    }

    ReferenceCountedPtr<D3D12Queue> presentQueue;
    if(desc.presentQueue)
    {
        assert(desc.graphicsQueue);
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        presentQueue = CreateD3D12Queue(device.Get(), queueDesc, QueueType::Graphics);
    }

    ReferenceCountedPtr<D3D12Queue> computeQueue;
    if(desc.computeQueue)
    {
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        computeQueue = CreateD3D12Queue(device.Get(), queueDesc, QueueType::Compute);
    }

    ReferenceCountedPtr<D3D12Queue> copyQueue;
    if(desc.copyQueue)
    {
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        copyQueue = CreateD3D12Queue(device.Get(), queueDesc, QueueType::Copy);
    }

    // Swapchain

    ComPtr<IDXGISwapChain3> swapchain;
    std::vector<TextureRef> swapchainImages;
    uint32_t currentSwapchainImageIndex;
    {
        const HWND windowHandle = reinterpret_cast<HWND>(desc.swapchainWindow->GetWin32WindowHandle());
        DXGI_SWAP_CHAIN_DESC swapChainDesc =
        {
            .BufferDesc = DXGI_MODE_DESC
            {
                .Width            = static_cast<UINT>(desc.swapchainWindow->GetFramebufferSize().x),
                .Height           = static_cast<UINT>(desc.swapchainWindow->GetFramebufferSize().y),
                .RefreshRate      = DXGI_RATIONAL{ 60, 1 },
                .Format           = FormatToD3D12TextureFormat(desc.swapchainFormat),
                .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                .Scaling          = DXGI_MODE_SCALING_UNSPECIFIED
            },
            .SampleDesc   = DXGI_SAMPLE_DESC{ 1, 0 },
            .BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT | (desc.enableSwapchainImageUAV ? DXGI_USAGE_UNORDERED_ACCESS : 0),
            .BufferCount  = static_cast<UINT>(desc.swapchainImageCount),
            .OutputWindow = windowHandle,
            .Windowed     = true,
            .SwapEffect   = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .Flags        = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
        };

        ComPtr<IDXGISwapChain> swapchain0;
        RTRC_D3D12_FAIL_MSG(
            factory->CreateSwapChain(
                (presentQueue ? presentQueue : graphicsQueue)->GetNativeQueue(),
                &swapChainDesc, swapchain0.GetAddressOf()),
            "Failed to create dxgi swapchain");
        RTRC_D3D12_FAIL_MSG(
            swapchain0->QueryInterface(IID_PPV_ARGS(swapchain.GetAddressOf())),
            "Failed to convert dxgi swapchain to dxgi swapchain 3");

        const TextureDesc imageDesc =
        {
            .format = desc.swapchainFormat,
            .width  = desc.swapchainWindow->GetFramebufferSize().x,
            .height = desc.swapchainWindow->GetFramebufferSize().y,
            .usage  = TextureUsage::RenderTarget |
                      (desc.enableSwapchainImageUAV ? TextureUsage::UnorderedAccess: TextureUsage::None),
        };

        swapchainImages.resize(desc.swapchainImageCount);
        for(uint32_t i = 0; i < desc.swapchainImageCount; ++i)
        {
            ComPtr<ID3D12Resource> image;
            RTRC_D3D12_FAIL_MSG(
                swapchain_->GetBuffer(i, IID_PPV_ARGS(image.GetAddressOf())),
                "Fail to get directx12 swapchain image");
            swapchainImages[i] = MakeReferenceCountedPtr<D3D12Texture>(
                ret, std::move(image), D3D12MemoryAllocation{ nullptr }, imageDesc);
            swapchainImages[i]->SetName(std::format("SwapChainImage{}", i));
        }
        currentSwapchainImageIndex = swapchain->GetCurrentBackBufferIndex();
    }

    // Descriptor heaps

    std::unique_ptr<D3D12CPUDescriptorHeap> CPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    for(int type = 0; type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++type)
    {
        CPUDescriptorHeaps[type] = std::make_unique<D3D12CPUDescriptorHeap>(
            device.Get(), 2048, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(type));
    }

    // Allocator

    ComPtr<D3D12MA::Allocator> allocator;
    const D3D12MA::ALLOCATOR_DESC allocatorDesc =
    {
        .Flags                = D3D12MA::ALLOCATOR_FLAG_NONE,
        .pDevice              = device.Get(),
        .PreferredBlockSize   = 0,
        .pAllocationCallbacks = RTRC_D3D12_ALLOC,
        .pAdapter             = adapter.Get()
    };
    RTRC_D3D12_FAIL_MSG(
        CreateAllocator(&allocatorDesc, allocator.GetAddressOf()),
        "Failed to initialize directx12 memory allocator");

    // Finalize

    ret->device_                     = device;
    ret->swapchain_                  = std::move(swapchain);
    ret->swapchainImages_            = std::move(swapchainImages);
    ret->swapchainSyncInterval_      = desc.vsync ? 1 : 0;
    ret->currentSwapchainImageIndex_ = currentSwapchainImageIndex;

    ret->allocator_ = std::move(allocator);

    ret->graphicsQueue_ = std::move(graphicsQueue);
    ret->presentQueue_  = std::move(presentQueue);
    ret->computeQueue_  = std::move(computeQueue);
    ret->copyQueue_     = std::move(copyQueue);

    for(size_t i = 0; i < GetArraySize(CPUDescriptorHeaps); ++i)
    {
        ret->CPUDescriptorHeaps_[i] = std::move(CPUDescriptorHeaps[i]);
    }

    return ret;
}

void D3D12Device::BeginThreadedMode()
{
    assert(!isInThreadedMode_);
    assert(threadContexts_.empty());
    isInThreadedMode_ = true;
}

void D3D12Device::EndThreadedMode()
{
    assert(isInThreadedMode_);
    isInThreadedMode_ = false;
    threadContexts_.clear();
}

QueueRef D3D12Device::GetQueue(QueueType queue)
{
    switch(queue)
    {
    case QueueType::Graphics: return graphicsQueue_;
    case QueueType::Compute:  return computeQueue_;
    case QueueType::Copy:     return copyQueue_;
    }
    Unreachable();
}

TextureRef D3D12Device::GetSwapchainImage()
{
    return swapchainImages_[currentSwapchainImageIndex_];
}

bool D3D12Device::Present()
{
    const bool ret = SUCCEEDED(swapchain_->Present(swapchainSyncInterval_, 0));
    currentSwapchainImageIndex_ = swapchain_->GetCurrentBackBufferIndex();
    return ret;
}

TextureRef D3D12Device::CreateTexture(const TextureDesc &desc)
{
    D3D12_RESOURCE_DESC d3d12Desc = {};

    d3d12Desc.Alignment          = 0;
    d3d12Desc.MipLevels          = static_cast<UINT16>(desc.mipLevels);
    d3d12Desc.Format             = FormatToD3D12TextureFormat(desc.format);
    d3d12Desc.SampleDesc.Count   = desc.MSAASampleCount;
    d3d12Desc.SampleDesc.Quality = desc.MSAASampleQuality;
    d3d12Desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    if(desc.usage.Contains(TextureUsageBit::RenderTarget))
    {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if(desc.usage.Contains(TextureUsageBit::DepthStencil))
    {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if(desc.usage.Contains(TextureUsageBit::UnorderedAccess))
    {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if(!desc.usage.Contains(TextureUsageBit::ShaderResource))
    {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }
    if(desc.queueAccessMode == QueueAccessMode::Concurrent)
    {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
    }

    if(desc.dimension == TextureDimension::Tex1D)
    {
        assert(desc.height == 1 && desc.depth == 1);
        d3d12Desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        d3d12Desc.Width            = desc.width;
        d3d12Desc.Height           = 1;
        d3d12Desc.DepthOrArraySize = static_cast<UINT16>(desc.arrayLayers);
    }
    else if(desc.dimension == TextureDimension::Tex2D)
    {
        assert(desc.depth == 1);
        d3d12Desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        d3d12Desc.Width            = desc.width;
        d3d12Desc.Height           = desc.height;
        d3d12Desc.DepthOrArraySize = static_cast<UINT16>(desc.arrayLayers);
    }
    else
    {
        assert(desc.dimension == TextureDimension::Tex3D);
        assert(desc.arrayLayers == 1);
        d3d12Desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        d3d12Desc.Width            = desc.width;
        d3d12Desc.Height           = desc.height;
        d3d12Desc.DepthOrArraySize = static_cast<UINT16>(desc.depth);
    }

    const D3D12MA::ALLOCATION_DESC allocDesc =
    {
        .HeapType = D3D12_HEAP_TYPE_DEFAULT // Upload/readback currently use staging buffers
    };

    D3D12_CLEAR_VALUE clearValue = {}, *pClearValue = nullptr;
    if(desc.fastClearValue.has_value())
    {
        clearValue.Format = d3d12Desc.Format;
        pClearValue = &clearValue;
        if(auto c = desc.fastClearValue->AsIf<ColorClearValue>())
        {
            clearValue.Color[0] = c->r;
            clearValue.Color[1] = c->g;
            clearValue.Color[2] = c->b;
            clearValue.Color[3] = c->a;
        }
        else
        {
            auto d = desc.fastClearValue->AsIf<DepthStencilClearValue>();
            assert(d);
            clearValue.DepthStencil.Depth = d->depth;
            clearValue.DepthStencil.Stencil = d->stencil;
        }
    }

    ComPtr<D3D12MA::Allocation> rawAlloc;
    ComPtr<ID3D12Resource> resource;
    RTRC_D3D12_FAIL_MSG(
        allocator_->CreateResource(
            &allocDesc, &d3d12Desc, D3D12_RESOURCE_STATE_COMMON, pClearValue,
            rawAlloc.GetAddressOf(), IID_PPV_ARGS(resource.GetAddressOf())),
        "Failed to create d3d12 texture resource");

    D3D12MemoryAllocation alloc = { std::move(rawAlloc) };
    return MakeReferenceCountedPtr<D3D12Texture>(this, std::move(resource), std::move(alloc), desc);
}

BufferRef D3D12Device::CreateBuffer(const BufferDesc &desc)
{
    D3D12_RESOURCE_DESC d3d12Desc = {};
    d3d12Desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    d3d12Desc.Alignment        = 0;
    d3d12Desc.Width            = desc.size;
    d3d12Desc.Height           = 1;
    d3d12Desc.DepthOrArraySize = 1;
    d3d12Desc.MipLevels        = 0;
    d3d12Desc.Format           = DXGI_FORMAT_UNKNOWN;
    d3d12Desc.SampleDesc       = { 1, 0 };
    d3d12Desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if(desc.usage.Contains(BufferUsageBit::UnorderedAccess))
    {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if(!desc.usage.Contains(BufferUsageBit::ShaderResource))
    {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }
    if(desc.queueAccessMode == QueueAccessMode::Concurrent)
    {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
    }

    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
    if(desc.hostAccessMode == BufferHostAccessMode::Upload)
    {
        heapType = D3D12_HEAP_TYPE_UPLOAD;
    }
    else if(desc.hostAccessMode == BufferHostAccessMode::Readback)
    {
        heapType = D3D12_HEAP_TYPE_READBACK;
    }
    const D3D12MA::ALLOCATION_DESC allocDesc = { .HeapType = heapType };

    ComPtr<D3D12MA::Allocation> rawAlloc;
    ComPtr<ID3D12Resource> resource;
    RTRC_D3D12_FAIL_MSG(
        allocator_->CreateResource(
            &allocDesc, &d3d12Desc, D3D12_RESOURCE_STATE_COMMON, nullptr,
            rawAlloc.GetAddressOf(), IID_PPV_ARGS(resource.GetAddressOf())),
        "Failed to create d3d12 buffer resource");

    D3D12MemoryAllocation alloc = { std::move(rawAlloc) };
    return MakeReferenceCountedPtr<D3D12Buffer>(this, std::move(resource), std::move(alloc), desc);
}

RTRC_RHI_D3D12_END
