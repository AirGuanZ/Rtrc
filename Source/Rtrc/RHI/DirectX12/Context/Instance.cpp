#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12_agility/d3d12.h>
#include <dxgi1_4.h>
#include <dxgidebug.h>

#include <Rtrc/RHI/DirectX12/Context/Instance.h>
#include <Rtrc/RHI/DirectX12/Context/Device.h>

RTRC_RHI_BEGIN

void InitializeDirectX12Backend()
{
    // Do nothing
}

UPtr<Instance> CreateDirectX12Instance(const DirectX12InstanceDesc &desc)
{
    using namespace D3D12;

    if(desc.debugMode)
    {
        ComPtr<ID3D12Debug> debugController;
        if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
        {
            debugController->EnableDebugLayer();
            if(desc.gpuValidation)
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

    return MakeUPtr<DirectX12Instance>(desc);
}

RTRC_RHI_END

RTRC_RHI_D3D12_BEGIN

DirectX12Instance::DirectX12Instance(DirectX12InstanceDesc desc)
    : desc_(desc)
{
    
}

UPtr<Device> DirectX12Instance::CreateDevice(const DeviceDesc &desc)
{
    ComPtr<IDXGIFactory4> factory;
    RTRC_D3D12_FAIL_MSG(
        CreateDXGIFactory(IID_PPV_ARGS(factory.GetAddressOf())),
        "Fail to create dxgi factory");

    ComPtr<IDXGIAdapter> adapter;
    RTRC_D3D12_FAIL_MSG(
        factory->EnumAdapters(0, adapter.GetAddressOf()),
        "Fail to get dxgi adapter");

    ComPtr<ID3D12Device5> device;
    RTRC_D3D12_FAIL_MSG(
        D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(device.GetAddressOf())),
        "Fail to create directx12 device");

    D3D12_COMMAND_QUEUE_DESC queueDesc;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    DirectX12Device::Queues queues;
    if(desc.graphicsQueue)
    {
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        RTRC_D3D12_FAIL_MSG(
            device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(queues.graphicsQueue.GetAddressOf())),
            "Fail to create directx12 graphics queue");
    }
    if(desc.computeQueue)
    {
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        RTRC_D3D12_FAIL_MSG(
            device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(queues.computeQueue.GetAddressOf())),
            "Fail to create directx12 compute queue");
    }
    if(desc.transferQueue)
    {
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        RTRC_D3D12_FAIL_MSG(
            device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(queues.copyQueue.GetAddressOf())),
            "Fail to create directx12 copy queue");
    }

    return MakeUPtr<DirectX12Device>(
        std::move(device), queues, std::move(factory), std::move(adapter), desc.supportSwapchain);
}

RTRC_RHI_D3D12_END
