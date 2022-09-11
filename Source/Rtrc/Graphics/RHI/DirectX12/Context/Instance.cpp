#include <dxgi1_4.h>
#include <dxgidebug.h>

#include <Rtrc/Graphics/RHI/DirectX12/Context/Device.h>
#include <Rtrc/Graphics/RHI/DirectX12/Context/Instance.h>

RTRC_RHI_BEGIN

void InitializeDirectX12Backend()
{
    // do nothing
}

Ptr<Instance> CreateDirectX12Instance(const DirectX12InstanceDesc &desc)
{
    using namespace DirectX12;

    if(desc.debugMode)
    {
        ComPtr<ID3D12Debug> debugController;
        if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
        {
            debugController->EnableDebugLayer();
            if(desc.gpuValidation)
            {
                ComPtr<ID3D12Debug1> debug1;
                D3D12_FAIL_MSG(
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

    return MakePtr<DirectX12Instance>();
}

RTRC_RHI_END

RTRC_RHI_DIRECTX12_BEGIN

Ptr<Device> DirectX12Instance::CreateDevice(const DeviceDesc &desc)
{
    ComPtr<IDXGIFactory4> factory;
    D3D12_FAIL_MSG(
        CreateDXGIFactory(IID_PPV_ARGS(factory.GetAddressOf())),
        "failed to create dxgi factory");

    ComPtr<IDXGIAdapter> adaptor;
    D3D12_FAIL_MSG(
        factory->EnumAdapters(0, adaptor.GetAddressOf()),
        "failed to get adaptor 0");

    ComPtr<ID3D12Device> device;
    D3D12_FAIL_MSG(
        D3D12CreateDevice(adaptor.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(device.GetAddressOf())),
        "failed to create d3d12 device");

    return MakePtr<DirectX12Device>(desc, std::move(device));
}

RTRC_RHI_DIRECTX12_END
