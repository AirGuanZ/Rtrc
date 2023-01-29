#include <dxgidebug.h>

#include <Rtrc/Graphics/RHI/DirectX12/Context/Instance.h>

RTRC_RHI_BEGIN

void InitializeDirectX12Backend()
{
    // Do nothing
}

Ptr<Instance> CreateDirectX12Instance(const DirectX12InstanceDesc &desc)
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

    return MakePtr<DirectX12Instance>(desc);
}

RTRC_RHI_END

RTRC_RHI_D3D12_BEGIN

DirectX12Instance::DirectX12Instance(DirectX12InstanceDesc desc)
    : desc_(desc)
{
    
}

Ptr<Device> DirectX12Instance::CreateDevice(const DeviceDesc &desc)
{
    // TODO
    return {};
}

RTRC_RHI_D3D12_END
