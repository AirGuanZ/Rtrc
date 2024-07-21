#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12_agility/d3d12.h>
#include <d3d12_agility/d3dx12/d3dx12.h>
#include <WinPixEventRuntime/pix3.h>

#include <Rtrc/RHI/DirectX12/Context/Device.h>
#include <Rtrc/RHI/DirectX12/Pipeline/BindingGroup.h>
#include <Rtrc/RHI/DirectX12/Pipeline/ComputePipeline.h>
#include <Rtrc/RHI/DirectX12/Pipeline/GraphicsPipeline.h>
#include <Rtrc/RHI/DirectX12/Pipeline/RayTracingPipeline.h>
#include <Rtrc/RHI/DirectX12/Pipeline/WorkGraphPipeline.h>
#include <Rtrc/RHI/DirectX12/Queue/CommandBuffer.h>
#include <Rtrc/RHI/DirectX12/Queue/CommandPool.h>
#include <Rtrc/RHI/DirectX12/RayTracing/BlasPrebuildInfo.h>
#include <Rtrc/RHI/DirectX12/RayTracing/TlasPrebuildInfo.h>
#include <Rtrc/RHI/DirectX12/Resource/Buffer.h>
#include <Rtrc/RHI/DirectX12/Resource/Texture.h>
#include <Rtrc/RHI/DirectX12/Resource/TextureView.h>
#include <Rtrc/Core/Enumerate.h>
#include <Rtrc/Core/Unreachable.h>

RTRC_RHI_D3D12_BEGIN

DirectX12CommandBuffer::DirectX12CommandBuffer(
    DirectX12Device                    *device,
    DirectX12CommandPool               *pool,
    ComPtr<ID3D12GraphicsCommandList10> commandList)
    : device_(device)
    , pool_(pool)
    , commandList_(std::move(commandList))
    , currentGraphicsRootSignature_(nullptr)
    , currentComputeRootSignature_(nullptr)
{
    
}

void DirectX12CommandBuffer::Begin()
{
    RTRC_D3D12_FAIL_MSG(
        commandList_->Reset(pool_->_internalGetNativeAllocator().Get(), nullptr),
        "Fail to reset directx12 command list");

    if(pool_->GetType() != QueueType::Transfer)
    {
        ID3D12DescriptorHeap *descHeaps[2] =
        {
            device_->_internalGetGPUDescriptorHeap_CbvSrvUav(),
            device_->_internalGetGPUDescriptorHeap_Sampler()
        };
        commandList_->SetDescriptorHeaps(2, descHeaps);
    }

    currentGraphicsRootSignature_ = nullptr;
    currentComputeRootSignature_ = nullptr;
}

void DirectX12CommandBuffer::End()
{
    commandList_->Close();
}

void DirectX12CommandBuffer::BeginRenderPass(
    Span<ColorAttachment>         colorAttachments,
    const DepthStencilAttachment &depthStencilAttachment)
{
    StaticVector<D3D12_CPU_DESCRIPTOR_HANDLE, 8> rtvs(colorAttachments.size());
    for(size_t i = 0; i < rtvs.size(); ++i)
    {
        const auto view = static_cast<DirectX12TextureRtv*>(colorAttachments[i].renderTargetView.Get());
        rtvs[i] = view->_internalGetDescriptorHandle();
        if(colorAttachments[i].loadOp == AttachmentLoadOp::Clear)
        {
            commandList_->ClearRenderTargetView(rtvs[i], &colorAttachments[i].clearValue.r, 0, nullptr);
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsv;
    if(depthStencilAttachment.depthStencilView)
    {
        const auto view = static_cast<DirectX12TextureDsv *>(depthStencilAttachment.depthStencilView.Get());
        dsv = view->_internalGetDescriptorHandle();
        if(depthStencilAttachment.loadOp == AttachmentLoadOp::Clear)
        {
            commandList_->ClearDepthStencilView(
                dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                depthStencilAttachment.clearValue.depth, depthStencilAttachment.clearValue.stencil,
                0, nullptr);
        }
    }

    commandList_->OMSetRenderTargets(
        static_cast<UINT>(rtvs.size()), rtvs.GetData(), false,
        depthStencilAttachment.depthStencilView ? &dsv : nullptr);
}

void DirectX12CommandBuffer::EndRenderPass()
{
    // Do nothing
}

void DirectX12CommandBuffer::BindPipeline(const OPtr<GraphicsPipeline> &pipeline)
{
    const auto d3dPipeline = static_cast<DirectX12GraphicsPipeline*>(pipeline.Get());
    currentGraphicsPipeline_ = pipeline;
    commandList_->SetPipelineState(d3dPipeline->_internalGetNativePipelineState().Get());
    if(auto s = d3dPipeline->_internalGetRootSignature().Get(); currentGraphicsRootSignature_ != s)
    {
        commandList_->SetGraphicsRootSignature(s);
        currentGraphicsRootSignature_ = s;
    }
    if(auto &s = d3dPipeline->_internalGetStaticViewports(); s)
    {
        commandList_->RSSetViewports(static_cast<UINT>(s->size()), s->data());
    }
    if(auto &s = d3dPipeline->_internalGetStaticScissors(); s)
    {
        commandList_->RSSetScissorRects(static_cast<UINT>(s->size()), s->data());
    }
    commandList_->IASetPrimitiveTopology(d3dPipeline->_internalGetTopology());
}

void DirectX12CommandBuffer::BindPipeline(const OPtr<ComputePipeline> &pipeline)
{
    const auto d3dPipeline = static_cast<DirectX12ComputePipeline*>(pipeline.Get());
    currentComputePipeline_ = pipeline;

#if RTRC_RHI_D3D12_USE_GENERIC_PROGRAM_FOR_COMPUTE_PIPELINE

    const D3D12_SET_PROGRAM_DESC setProgramDesc =
    {
        .Type = D3D12_PROGRAM_TYPE_GENERIC_PIPELINE,
        .GenericPipeline = D3D12_SET_GENERIC_PIPELINE_DESC
        {
            .ProgramIdentifier = d3dPipeline->_internalGetProgramIdentifier()
        }
    };
    commandList_->SetProgram(&setProgramDesc);

#else

    commandList_->SetPipelineState(d3dPipeline->_internalGetNativePipelineState().Get());

#endif

    if(auto s = d3dPipeline->_internalGetRootSignature().Get(); currentComputeRootSignature_ != s)
    {
        commandList_->SetComputeRootSignature(s);
        currentComputeRootSignature_ = s;
    }
}

void DirectX12CommandBuffer::BindPipeline(const OPtr<RayTracingPipeline> &pipeline)
{
    auto d3dPipeline = static_cast<DirectX12RayTracingPipeline*>(pipeline.Get());
    currentRayTracingPipeline_ = pipeline;
    commandList_->SetPipelineState1(d3dPipeline->_internalGetStateObject().Get());

    if(auto s = d3dPipeline->_internalGetRootSignature().Get(); currentComputeRootSignature_ != s)
    {
        commandList_->SetComputeRootSignature(s);
        currentComputeRootSignature_ = s;
    }
}

void DirectX12CommandBuffer::BindPipeline(const OPtr<WorkGraphPipeline> &pipeline)
{
    auto d3dPipeline = static_cast<DirectX12WorkGraphPipeline *>(pipeline.Get());
    currentWorkGraphPipeline_ = pipeline;
    commandList_->SetPipelineState1(d3dPipeline->_internalGetStateObject().Get());

    const D3D12_SET_PROGRAM_DESC setProgramDesc =
    {
        .Type = D3D12_PROGRAM_TYPE_GENERIC_PIPELINE,
        .GenericPipeline = D3D12_SET_GENERIC_PIPELINE_DESC
        {
            .ProgramIdentifier = d3dPipeline->_internalGetProgramIdentifier()
        }
    };
    commandList_->SetProgram(&setProgramDesc);

    auto d3dBindingLayout = static_cast<DirectX12BindingLayout *>(d3dPipeline->GetDesc().bindingLayout.Get());
    if(auto s = d3dBindingLayout->_internalGetRootSignature(false).Get(); currentComputeRootSignature_ != s)
    {
        commandList_->SetComputeRootSignature(s);
        currentComputeRootSignature_ = s;
    }
}

void DirectX12CommandBuffer::BindGroupsToGraphicsPipeline(int startIndex, Span<OPtr<BindingGroup>> groups)
{
    for(auto &&[i, group] : Enumerate(groups))
    {
        BindGroupToGraphicsPipeline(static_cast<int>(startIndex + i), group);
    }
}

void DirectX12CommandBuffer::BindGroupsToComputePipeline(int startIndex, Span<OPtr<BindingGroup>> groups)
{
    for(auto &&[i, group] : Enumerate(groups))
    {
        BindGroupToComputePipeline(static_cast<int>(startIndex + i), group);
    }
}

void DirectX12CommandBuffer::BindGroupsToRayTracingPipeline(int startIndex, Span<OPtr<BindingGroup>> groups)
{
    for(auto &&[i, group] : Enumerate(groups))
    {
        BindGroupToRayTracingPipeline(static_cast<int>(startIndex + i), group);
    }
}

void DirectX12CommandBuffer::BindGroupsToWorkGraphPipeline(int startIndex, Span<OPtr<BindingGroup>> groups)
{
    for(auto &&[i, group] : Enumerate(groups))
    {
        BindGroupToWorkGraphPipeline(static_cast<int>(startIndex + i), group);
    }
}

void DirectX12CommandBuffer::BindGroupToGraphicsPipeline(int index, const OPtr<BindingGroup> &group)
{
    const auto bindingLayout = static_cast<DirectX12BindingLayout*>(currentGraphicsPipeline_->GetBindingLayout().Get());
    const auto d3dGroup = static_cast<DirectX12BindingGroup *>(group.Get());
    const int firstRootParamIndex = bindingLayout->_internalGetRootParamIndex(index);
    for(auto &&[i, table] : Enumerate(d3dGroup->_internalGetDescriptorTables()))
    {
        commandList_->SetGraphicsRootDescriptorTable(static_cast<UINT>(firstRootParamIndex + i), table.gpuHandle);
    }

    for(auto &alias : bindingLayout->_internalGetUnboundedResourceArrayAliases(index))
    {
        auto &table = d3dGroup->_internalGetDescriptorTables()[alias.srcTableIndex];
        const D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { table.gpuHandle.ptr + alias.offsetInSrcTable };
        commandList_->SetGraphicsRootDescriptorTable(static_cast<UINT>(alias.rootParamIndex), gpuHandle);
    }
}

void DirectX12CommandBuffer::BindGroupToComputePipeline(int index, const OPtr<BindingGroup> &group)
{
    const auto bindingLayout = static_cast<DirectX12BindingLayout*>(currentComputePipeline_->GetBindingLayout().Get());
    const auto d3dGroup = static_cast<DirectX12BindingGroup *>(group.Get());
    const int firstRootParamIndex = bindingLayout->_internalGetRootParamIndex(index);
    for(auto &&[i, table] : Enumerate(d3dGroup->_internalGetDescriptorTables()))
    {
        commandList_->SetComputeRootDescriptorTable(static_cast<UINT>(firstRootParamIndex + i), table.gpuHandle);
    }

    for(auto &alias : bindingLayout->_internalGetUnboundedResourceArrayAliases(index))
    {
        auto &table = d3dGroup->_internalGetDescriptorTables()[alias.srcTableIndex];
        const D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { table.gpuHandle.ptr + alias.offsetInSrcTable };
        commandList_->SetComputeRootDescriptorTable(static_cast<UINT>(alias.rootParamIndex), gpuHandle);
    }
}

void DirectX12CommandBuffer::BindGroupToRayTracingPipeline(int index, const OPtr<BindingGroup> &group)
{
    const auto bindingLayout = static_cast<DirectX12BindingLayout *>(currentRayTracingPipeline_->GetBindingLayout().Get());
    const auto d3dGroup = static_cast<DirectX12BindingGroup *>(group.Get());
    const int firstRootParamIndex = bindingLayout->_internalGetRootParamIndex(index);
    for(auto &&[i, table] : Enumerate(d3dGroup->_internalGetDescriptorTables()))
    {
        commandList_->SetComputeRootDescriptorTable(static_cast<UINT>(firstRootParamIndex + i), table.gpuHandle);
    }

    for(auto &alias : bindingLayout->_internalGetUnboundedResourceArrayAliases(index))
    {
        auto &table = d3dGroup->_internalGetDescriptorTables()[alias.srcTableIndex];
        const D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { table.gpuHandle.ptr + alias.offsetInSrcTable };
        commandList_->SetComputeRootDescriptorTable(static_cast<UINT>(alias.rootParamIndex), gpuHandle);
    }
}

void DirectX12CommandBuffer::BindGroupToWorkGraphPipeline(int index, const OPtr<BindingGroup> &group)
{
    const auto bindingLayout = static_cast<DirectX12BindingLayout *>(currentWorkGraphPipeline_->GetDesc().bindingLayout.Get());
    const auto d3dGroup = static_cast<DirectX12BindingGroup *>(group.Get());
    const int firstRootParamIndex = bindingLayout->_internalGetRootParamIndex(index);
    for(auto &&[i, table] : Enumerate(d3dGroup->_internalGetDescriptorTables()))
    {
        commandList_->SetComputeRootDescriptorTable(static_cast<UINT>(firstRootParamIndex + i), table.gpuHandle);
    }

    for(auto &alias : bindingLayout->_internalGetUnboundedResourceArrayAliases(index))
    {
        auto &table = d3dGroup->_internalGetDescriptorTables()[alias.srcTableIndex];
        const D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = { table.gpuHandle.ptr + alias.offsetInSrcTable };
        commandList_->SetComputeRootDescriptorTable(static_cast<UINT>(alias.rootParamIndex), gpuHandle);
    }
}

void DirectX12CommandBuffer::SetViewports(Span<Viewport> viewports)
{
    std::vector<D3D12_VIEWPORT> d3dViewports;
    d3dViewports.reserve(viewports.size());
    std::ranges::transform(
        viewports, std::back_inserter(d3dViewports),
        [](const Viewport &v) { return TranslateViewport(v); });
    commandList_->RSSetViewports(static_cast<UINT>(d3dViewports.size()), d3dViewports.data());
}

void DirectX12CommandBuffer::SetScissors(Span<Scissor> scissors)
{
    std::vector<D3D12_RECT> d3dScissors;
    d3dScissors.reserve(scissors.size());
    std::ranges::transform(
        scissors, std::back_inserter(d3dScissors),
        [](const Scissor &s) { return TranslateScissor(s); });
    commandList_->RSSetScissorRects(static_cast<UINT>(d3dScissors.size()), d3dScissors.data());
}

void DirectX12CommandBuffer::SetViewportsWithCount(Span<Viewport> viewports)
{
    SetViewports(viewports);
}

void DirectX12CommandBuffer::SetScissorsWithCount(Span<Scissor> scissors)
{
    SetScissors(scissors);
}

void DirectX12CommandBuffer::SetVertexBuffer(int slot, Span<BufferRPtr> buffers, Span<size_t> byteOffsets, Span<size_t> byteStrides)
{
    std::vector<D3D12_VERTEX_BUFFER_VIEW> views;
    views.reserve(buffers.size());
    for(auto &&[i, buffer] : Enumerate(buffers))
    {
        const size_t byteOffset = byteOffsets[i];
        const size_t stride = byteStrides[i];
        const size_t size = buffer->GetDesc().size - byteOffsets[i];
        views.push_back(
        {
            .BufferLocation = D3D12_GPU_VIRTUAL_ADDRESS{ buffer->GetDeviceAddress().address + byteOffset },
            .SizeInBytes    = static_cast<UINT>(size - (size % stride)),
            .StrideInBytes  = static_cast<UINT>(stride)
        });
    }
    commandList_->IASetVertexBuffers(slot, static_cast<UINT>(views.size()), views.data());
}

void DirectX12CommandBuffer::SetIndexBuffer(const BufferRPtr &buffer, size_t byteOffset, IndexFormat format)
{
    DXGI_FORMAT d3dFormat; size_t stride;
    if(format == IndexFormat::UInt16)
    {
        d3dFormat = DXGI_FORMAT_R16_UINT;
        stride = sizeof(uint16_t);
    }
    else if(format == IndexFormat::UInt32)
    {
        d3dFormat = DXGI_FORMAT_R32_UINT;
        stride = sizeof(uint32_t);
    }
    else
    {
        Unreachable();
    }
    const size_t size = buffer->GetDesc().size - byteOffset;
    D3D12_INDEX_BUFFER_VIEW view =
    {
        .BufferLocation = D3D12_GPU_VIRTUAL_ADDRESS{ buffer->GetDeviceAddress().address + byteOffset },
        .SizeInBytes    = static_cast<UINT>(size - (size % stride)),
        .Format         = d3dFormat
    };
    commandList_->IASetIndexBuffer(&view);
}

void DirectX12CommandBuffer::SetStencilReferenceValue(uint8_t value)
{
    commandList_->OMSetStencilRef(value);
}

void DirectX12CommandBuffer::SetGraphicsPushConstants(
    uint32_t rangeIndex, uint32_t offsetInRange, uint32_t size, const void *data)
{
    auto d3dBindingLayout = static_cast<DirectX12BindingLayout*>(currentGraphicsPipeline_->GetBindingLayout().Get());
    commandList_->SetGraphicsRoot32BitConstants(
        d3dBindingLayout->_internalGetFirstPushConstantRootParamIndex() + rangeIndex,
        size / sizeof(uint32_t), data, offsetInRange / sizeof(uint32_t));
}

void DirectX12CommandBuffer::SetComputePushConstants(
    uint32_t rangeIndex, uint32_t offsetInRange, uint32_t size, const void *data)
{
    auto d3dBindingLayout = static_cast<DirectX12BindingLayout *>(currentComputePipeline_->GetBindingLayout().Get());
    commandList_->SetComputeRoot32BitConstants(
        d3dBindingLayout->_internalGetFirstPushConstantRootParamIndex() + rangeIndex,
        size / sizeof(uint32_t), data, offsetInRange / sizeof(uint32_t));
}

void DirectX12CommandBuffer::Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance)
{
    commandList_->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void DirectX12CommandBuffer::DrawIndexed(int indexCount, int instanceCount, int firstIndex, int firstVertex, int firstInstance)
{
    commandList_->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
}

void DirectX12CommandBuffer::DispatchMesh(int groupCountX, int groupCountY, int groupCountZ)
{
    commandList_->DispatchMesh(groupCountX, groupCountY, groupCountZ);
}

void DirectX12CommandBuffer::Dispatch(int groupCountX, int groupCountY, int groupCountZ)
{
    commandList_->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void DirectX12CommandBuffer::DispatchNode(
    uint32_t entryIndex, uint32_t recordCount, uint32_t recordStride, const void *records)
{
    const D3D12_DISPATCH_GRAPH_DESC dispatchDesc =
    {
        .Mode         = D3D12_DISPATCH_MODE_NODE_CPU_INPUT,
        .NodeCPUInput = D3D12_NODE_CPU_INPUT
        {
            .EntrypointIndex     = entryIndex,
            .NumRecords          = recordCount,
            .pRecords            = records,
            .RecordStrideInBytes = recordStride
        }
    };
    commandList_->DispatchGraph(&dispatchDesc);
}

void DirectX12CommandBuffer::TraceRays(
    int                             rayCountX,
    int                             rayCountY,
    int                             rayCountZ,
    const ShaderBindingTableRegion &rayGenSbt,
    const ShaderBindingTableRegion &missSbt,
    const ShaderBindingTableRegion &hitSbt,
    const ShaderBindingTableRegion &callableSbt)
{
    D3D12_DISPATCH_RAYS_DESC desc;
    desc.RayGenerationShaderRecord.StartAddress = rayGenSbt.deviceAddress.address;
    desc.RayGenerationShaderRecord.SizeInBytes  = rayGenSbt.size;
    desc.MissShaderTable.StartAddress           = missSbt.deviceAddress.address;
    desc.MissShaderTable.SizeInBytes            = missSbt.size;
    desc.MissShaderTable.StrideInBytes          = missSbt.stride;
    desc.HitGroupTable.StartAddress             = hitSbt.deviceAddress.address;
    desc.HitGroupTable.SizeInBytes              = hitSbt.size;
    desc.HitGroupTable.StrideInBytes            = hitSbt.stride;
    desc.CallableShaderTable.StartAddress       = callableSbt.deviceAddress.address;
    desc.CallableShaderTable.SizeInBytes        = callableSbt.size;
    desc.CallableShaderTable.StrideInBytes      = callableSbt.stride;
    desc.Width                                  = static_cast<UINT>(rayCountX);
    desc.Height                                 = static_cast<UINT>(rayCountY);
    desc.Depth                                  = static_cast<UINT>(rayCountZ);
    commandList_->DispatchRays(&desc);
}

void DirectX12CommandBuffer::DispatchIndirect(const BufferRPtr &buffer, size_t byteOffset)
{
    auto d3dBuffer = static_cast<DirectX12Buffer*>(buffer.Get())->_internalGetNativeBuffer().Get();
    auto commandSignature = device_->_internalGetIndirectDispatchCommandSignature();
    commandList_->ExecuteIndirect(commandSignature, 1, d3dBuffer, byteOffset, nullptr, 0);
}

void DirectX12CommandBuffer::DrawIndexedIndirect(
    const BufferRPtr &buffer, uint32_t drawCount, size_t byteOffset, size_t byteStride)
{
    auto d3dBuffer = static_cast<DirectX12Buffer *>(buffer.Get())->_internalGetNativeBuffer().Get();
    auto commandSignature = device_->_internalGetIndirectDrawIndexedCommandSignature();
    if(byteStride == sizeof(D3D12_DRAW_INDEXED_ARGUMENTS))
    {
        commandList_->ExecuteIndirect(commandSignature, drawCount, d3dBuffer, byteOffset, nullptr, 0);
    }
    else
    {
        for(uint32_t i = 0; i < drawCount; ++i)
        {
            commandList_->ExecuteIndirect(commandSignature, 1, d3dBuffer, byteOffset, nullptr, 0);
            byteOffset += byteStride;
        }
    }
}

void DirectX12CommandBuffer::CopyBuffer(Buffer *dst, size_t dstOffset, Buffer *src, size_t srcOffset, size_t range)
{
    auto d3dDst = static_cast<DirectX12Buffer*>(dst)->_internalGetNativeBuffer().Get();
    auto d3dSrc = static_cast<DirectX12Buffer*>(src)->_internalGetNativeBuffer().Get();
    commandList_->CopyBufferRegion(d3dDst, dstOffset, d3dSrc, srcOffset, range);
}

void DirectX12CommandBuffer::CopyColorTexture(
    Texture *dst, uint32_t dstMipLevel, uint32_t dstArrayLayer,
    Texture *src, uint32_t srcMipLevel, uint32_t srcArrayLayer)
{
    auto d3dDst = static_cast<DirectX12Texture *>(dst)->_internalGetNativeTexture().Get();
    auto d3dSrc = static_cast<DirectX12Texture *>(src)->_internalGetNativeTexture().Get();
    const D3D12_TEXTURE_COPY_LOCATION dstLoc =
    {
        .pResource        = d3dDst,
        .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = D3D12CalcSubresource(
                                dstMipLevel, dstArrayLayer, 0, dst->GetMipLevels(), dst->GetArraySize())
    };
    const D3D12_TEXTURE_COPY_LOCATION srcLoc =
    {
        .pResource        = d3dSrc,
        .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = D3D12CalcSubresource(
                                srcMipLevel, srcArrayLayer, 0, src->GetMipLevels(), src->GetArraySize())
    };
    commandList_->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
}

void DirectX12CommandBuffer::CopyColorTexture(
    Texture* dst, uint32_t dstMipLevel, uint32_t dstArrayLayer, const Vector3u &dstOffset,
    Texture* src, uint32_t srcMipLevel, uint32_t srcArrayLayer, const Vector3u &srcOffset,
    const Vector3u &size)
{
    auto d3dDst = static_cast<DirectX12Texture *>(dst)->_internalGetNativeTexture().Get();
    auto d3dSrc = static_cast<DirectX12Texture *>(src)->_internalGetNativeTexture().Get();
    const D3D12_TEXTURE_COPY_LOCATION dstLoc =
    {
        .pResource        = d3dDst,
        .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = D3D12CalcSubresource(
                                dstMipLevel, dstArrayLayer, 0, dst->GetMipLevels(), dst->GetArraySize())
    };
    const D3D12_TEXTURE_COPY_LOCATION srcLoc =
    {
        .pResource        = d3dSrc,
        .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = D3D12CalcSubresource(
                                srcMipLevel, srcArrayLayer, 0, src->GetMipLevels(), src->GetArraySize())
    };
    const D3D12_BOX srcBox =
    {
        .left   = srcOffset.x,
        .top    = srcOffset.y,
        .front  = srcOffset.z,
        .right  = srcOffset.x + size.x,
        .bottom = srcOffset.y + size.y,
        .back   = srcOffset.z + size.z
    };
    commandList_->CopyTextureRegion(&dstLoc, dstOffset.x, dstOffset.y, dstOffset.z, &srcLoc, &srcBox);
}

void DirectX12CommandBuffer::CopyBufferToTexture(
    Texture *dst, uint32_t mipLevel, uint32_t arrayLayer, Buffer *src, size_t srcOffset, size_t srcRowBytes)
{
    assert(srcRowBytes % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT == 0 ||
           ((dst->GetHeight() >> mipLevel) <= 1 &&
            (dst->GetDepth() >> mipLevel) <= 1));

    assert(!HasDepthAspect(dst->GetFormat()) && !HasStencilAspect(dst->GetFormat()));
    const DXGI_FORMAT dstFormat = TranslateFormat(dst->GetFormat());

    auto d3dSrc = static_cast<DirectX12Buffer*>(src)->_internalGetNativeBuffer().Get();
    const D3D12_TEXTURE_COPY_LOCATION srcLoc =
    {
        .pResource       = d3dSrc,
        .Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
        .PlacedFootprint = D3D12_PLACED_SUBRESOURCE_FOOTPRINT
        {
            .Offset = srcOffset,
            .Footprint = D3D12_SUBRESOURCE_FOOTPRINT
            {
                .Format   = dstFormat,
                .Width    = (std::max)(1u, dst->GetWidth() >> mipLevel),
                .Height   = (std::max)(1u, dst->GetHeight() >> mipLevel),
                .Depth    = dst->GetDimension() == TextureDimension::Tex3D ?
                            (std::max)(1u, dst->GetDepth() >> mipLevel) : 1u,
                .RowPitch = static_cast<UINT>(srcRowBytes)
            }
        }
    };
    const D3D12_TEXTURE_COPY_LOCATION dstLoc =
    {
        .pResource        = static_cast<DirectX12Texture *>(dst)->_internalGetNativeTexture().Get(),
        .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = D3D12CalcSubresource(mipLevel, arrayLayer, 0, dst->GetMipLevels(), dst->GetArraySize())
    };
    commandList_->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
}

void DirectX12CommandBuffer::CopyTextureToBuffer(
    Buffer *dst, size_t dstOffset, size_t dstRowBytes, Texture *src, uint32_t mipLevel, uint32_t arrayLayer)
{
    assert(dstRowBytes % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT == 0 ||
           ((src->GetHeight() >> mipLevel) <= 1 &&
            (src->GetDepth() >> mipLevel) <= 1));

    assert(!HasDepthAspect(src->GetFormat()) && !HasStencilAspect(src->GetFormat()));
    const DXGI_FORMAT srcFormat = TranslateFormat(src->GetFormat());

    auto d3dDst = static_cast<DirectX12Buffer *>(dst)->_internalGetNativeBuffer().Get();
    const D3D12_TEXTURE_COPY_LOCATION dstLoc =
    {
        .pResource       = d3dDst,
        .Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
        .PlacedFootprint = D3D12_PLACED_SUBRESOURCE_FOOTPRINT
        {
            .Offset    = dstOffset,
            .Footprint = D3D12_SUBRESOURCE_FOOTPRINT
            {
                .Format   = srcFormat,
                .Width    = (std::max)(1u, src->GetWidth() >> mipLevel),
                .Height   = (std::max)(1u, src->GetHeight() >> mipLevel),
                .Depth    = src->GetDimension() == TextureDimension::Tex3D ?
                            (std::max)(1u, src->GetDepth() >> mipLevel) : 1u,
                .RowPitch = static_cast<UINT>(dstRowBytes)
            }
        }
    };
    const D3D12_TEXTURE_COPY_LOCATION srcLoc =
    {
        .pResource        = static_cast<DirectX12Texture *>(src)->_internalGetNativeTexture().Get(),
        .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = D3D12CalcSubresource(mipLevel, arrayLayer, 0, src->GetMipLevels(), src->GetArraySize())
    };
    commandList_->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
}

void DirectX12CommandBuffer::ClearColorTexture2D(Texture *dst, const ColorClearValue &clearValue)
{
    const TextureRtvDesc rtvDesc =
    {
        .format     = Format::Unknown,
        .mipLevel   = 0,
        .arrayLayer = 0
    };
    assert(dst->GetDesc().usage.Contains(TextureUsage::ClearDst));
    auto rtv = dst->CreateRtv(rtvDesc);
    auto d3dRtv = static_cast<DirectX12TextureRtv*>(rtv.Get())->_internalGetDescriptorHandle();
    const float rgba[4] = { clearValue.r, clearValue.g, clearValue.b, clearValue.a };
    const D3D12_RECT rect = { 0, 0, static_cast<LONG>(dst->GetWidth()), static_cast<LONG>(dst->GetHeight()) };
    commandList_->ClearRenderTargetView(d3dRtv, rgba, 1, &rect);
}

void DirectX12CommandBuffer::ClearDepthStencilTexture(
    Texture *dst, const DepthStencilClearValue &clearValue, bool depth, bool stencil)
{
    assert(depth || stencil);
    auto dsv = dst->CreateDsv();
    auto d3dDsv = static_cast<DirectX12TextureDsv*>(dsv.Get())->_internalGetDescriptorHandle();
    D3D12_CLEAR_FLAGS flags = {};
    if(depth)
    {
        flags |= D3D12_CLEAR_FLAG_DEPTH;
    }
    if(stencil)
    {
        flags |= D3D12_CLEAR_FLAG_STENCIL;
    }
    commandList_->ClearDepthStencilView(d3dDsv, flags, clearValue.depth, clearValue.stencil, 0, nullptr);
}

void DirectX12CommandBuffer::BeginDebugEvent(const DebugLabel &label)
{
    UINT color = PIX_COLOR(0, 0, 0);
    if(label.color)
    {
        auto ToU8 = [](float x) { return static_cast<BYTE>(std::clamp(x, 0.0f, 1.0f) * 255.0f); };
        color = PIX_COLOR(ToU8(label.color->x), ToU8(label.color->y), ToU8(label.color->z));
    }
    PIXBeginEvent(commandList_.Get(), color, "%s", label.name.c_str());
}

void DirectX12CommandBuffer::EndDebugEvent()
{
    PIXEndEvent(commandList_.Get());
}

void DirectX12CommandBuffer::BuildBlas(
    const BlasPrebuildInfoOPtr  &buildInfo,
    Span<RayTracingGeometryDesc> geometries,
    const BlasOPtr              &blas,
    BufferDeviceAddress          scratchBufferAddress)
{
    static_cast<DirectX12BlasPrebuildInfo *>(buildInfo.Get())
        ->_internalBuildBlas(commandList_.Get(), geometries, blas, scratchBufferAddress);
}

void DirectX12CommandBuffer::BuildTlas(
    const TlasPrebuildInfoOPtr        &buildInfo,
    const RayTracingInstanceArrayDesc &instances,
    const TlasOPtr                    &tlas,
    BufferDeviceAddress                scratchBufferAddress)
{
    static_cast<DirectX12TlasPrebuildInfo*>(buildInfo.Get())
        ->_internalBuildTlas(commandList_.Get(), instances, tlas, scratchBufferAddress);
}

void DirectX12CommandBuffer::ExecuteBarriersInternal(
    Span<GlobalMemoryBarrier>      globalMemoryBarriers,
    Span<TextureTransitionBarrier> textureTransitions,
    Span<BufferTransitionBarrier>  bufferTransitions,
    Span<TextureReleaseBarrier>    textureReleaseBarriers,
    Span<TextureAcquireBarrier>    textureAcquireBarriers)
{
    auto TranslateBarrierSubresources = [](const TexSubrscs &subrscs, Format format)
    {
        D3D12_BARRIER_SUBRESOURCE_RANGE ret;
        ret.IndexOrFirstMipLevel = subrscs.mipLevel;
        ret.NumMipLevels         = subrscs.levelCount;
        ret.FirstArraySlice      = subrscs.arrayLayer;
        ret.NumArraySlices       = subrscs.layerCount;
        ret.FirstPlane           = 0;
        ret.NumPlanes            = HasDepthAspect(format) && HasStencilAspect(format) ? 2 : 1;
        return ret;
    };

    const bool isOnCopyQueue = pool_->GetType() == QueueType::Transfer;
    auto LowerLayout = [&](D3D12_BARRIER_LAYOUT &layout)
    {
        if(isOnCopyQueue)
        {
            layout = D3D12_BARRIER_LAYOUT_COMMON;
        }
    };

    std::vector<D3D12_GLOBAL_BARRIER> d3dGlobalBarriers;
    d3dGlobalBarriers.reserve(globalMemoryBarriers.size());
    for(auto &b : globalMemoryBarriers)
    {
        auto &d3dBarrier = d3dGlobalBarriers.emplace_back();
        d3dBarrier.SyncBefore   = TranslateBarrierSync(b.beforeStages, Format::Unknown);
        d3dBarrier.SyncAfter    = TranslateBarrierSync(b.afterStages, Format::Unknown);
        d3dBarrier.AccessBefore = TranslateBarrierAccess(b.beforeAccesses);
        d3dBarrier.AccessAfter  = TranslateBarrierAccess(b.afterAccesses);
    }

    std::vector<D3D12_TEXTURE_BARRIER> d3dTextureBarriers;
    for(auto &b : textureTransitions)
    {
        const Format format = b.texture->GetFormat();

        auto &d3dBarrier = d3dTextureBarriers.emplace_back();
        d3dBarrier.pResource    = static_cast<DirectX12Texture *>(b.texture)->_internalGetNativeTexture().Get();
        d3dBarrier.LayoutBefore = TranslateTextureLayout(b.beforeLayout, format);
        d3dBarrier.LayoutAfter  = TranslateTextureLayout(b.afterLayout, format);
        d3dBarrier.SyncBefore   = TranslateBarrierSync(b.beforeStages, format);
        d3dBarrier.SyncAfter    = TranslateBarrierSync(b.afterStages, format);
        d3dBarrier.AccessBefore = TranslateBarrierAccess(b.beforeAccesses);
        d3dBarrier.AccessAfter  = TranslateBarrierAccess(b.afterAccesses);
        d3dBarrier.Subresources = TranslateBarrierSubresources(b.subresources, format);
        d3dBarrier.Flags        = D3D12_TEXTURE_BARRIER_FLAG_NONE;

        if(b.beforeLayout == TextureLayout::Present)
        {
            d3dBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_UNDEFINED;
            d3dBarrier.SyncBefore   = D3D12_BARRIER_SYNC_NONE;
            d3dBarrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
        }
        else if(b.afterLayout == TextureLayout::Present)
        {
            d3dBarrier.SyncBefore   = D3D12_BARRIER_SYNC_ALL;
            d3dBarrier.AccessBefore = D3D12_BARRIER_ACCESS_COMMON;
        }

        LowerLayout(d3dBarrier.LayoutBefore);
        LowerLayout(d3dBarrier.LayoutAfter);

        if(d3dBarrier.LayoutBefore == D3D12_BARRIER_LAYOUT_UNDEFINED)
        {
            d3dBarrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
        }

        if(d3dBarrier.AccessBefore & D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE)
        {
            d3dBarrier.AccessBefore &= ~D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
        }
        if(d3dBarrier.AccessAfter & D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE)
        {
            d3dBarrier.AccessAfter &= ~D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
        }

        if(d3dBarrier.LayoutBefore == D3D12_BARRIER_LAYOUT_UNDEFINED)
        {
            d3dBarrier.Flags |= D3D12_TEXTURE_BARRIER_FLAG_DISCARD;
        }

        if(b.texture->GetConcurrentAccessMode() == QueueConcurrentAccessMode::Concurrent)
        {
            if(d3dBarrier.LayoutBefore != D3D12_BARRIER_LAYOUT_UNDEFINED)
                d3dBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COMMON;
            d3dBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_COMMON;
        }
    }

    std::vector<D3D12_BUFFER_BARRIER> d3dBufferBarriers;
    for(auto &b : bufferTransitions)
    {
        auto &d3dBarrier = d3dBufferBarriers.emplace_back();
        d3dBarrier.pResource    = static_cast<DirectX12Buffer *>(b.buffer)->_internalGetNativeBuffer().Get();
        d3dBarrier.Size         = b.buffer->GetDesc().size;
        d3dBarrier.Offset       = 0;
        d3dBarrier.SyncBefore   = TranslateBarrierSync(b.beforeStages, Format::Unknown);
        d3dBarrier.SyncAfter    = TranslateBarrierSync(b.afterStages, Format::Unknown);
        d3dBarrier.AccessBefore = TranslateBarrierAccess(b.beforeAccesses);
        d3dBarrier.AccessAfter  = TranslateBarrierAccess(b.afterAccesses);

        if(b.beforeAccesses == ResourceAccess::BuildASScratch)
        {
            d3dBarrier.SyncBefore = D3D12_BARRIER_SYNC_COMPUTE_SHADING;
        }
        if(b.afterAccesses == ResourceAccess::BuildASScratch)
        {
            d3dBarrier.SyncAfter = D3D12_BARRIER_SYNC_COMPUTE_SHADING;
        }
    }

    for(auto &b : textureReleaseBarriers)
    {
        const bool shouldEmit = b.beforeLayout != b.afterLayout;
        const bool shouldEmitOnThisQueue = b.beforeQueue <= b.afterQueue;
        if(!shouldEmit || !shouldEmitOnThisQueue)
        {
            continue;
        }

        const Format format = b.texture->GetFormat();

        auto &d3dBarrier = d3dTextureBarriers.emplace_back();
        d3dBarrier.pResource    = static_cast<DirectX12Texture *>(b.texture)->_internalGetNativeTexture().Get();
        d3dBarrier.LayoutBefore = TranslateTextureLayout(b.beforeLayout, format);
        d3dBarrier.LayoutAfter  = TranslateTextureLayout(b.afterLayout, format);
        d3dBarrier.SyncBefore   = TranslateBarrierSync(b.beforeStages, format);
        d3dBarrier.SyncAfter    = TranslateBarrierSync(D3D12_BARRIER_SYNC_ALL, format);
        d3dBarrier.AccessBefore = TranslateBarrierAccess(b.beforeAccesses);
        d3dBarrier.AccessAfter  = TranslateBarrierAccess(D3D12_BARRIER_ACCESS_COMMON);
        d3dBarrier.Subresources = TranslateBarrierSubresources(b.subresources, format);
        d3dBarrier.Flags        = D3D12_TEXTURE_BARRIER_FLAG_NONE;

        assert(b.beforeQueue != QueueType::Transfer);
        if(b.afterQueue == QueueType::Transfer)
        {
            d3dBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_COMMON;
        }

        if(d3dBarrier.LayoutBefore == D3D12_BARRIER_LAYOUT_UNDEFINED)
        {
            d3dBarrier.Flags |= D3D12_TEXTURE_BARRIER_FLAG_DISCARD;
        }

        if(b.texture->GetConcurrentAccessMode() == QueueConcurrentAccessMode::Concurrent)
        {
            if(d3dBarrier.LayoutBefore != D3D12_BARRIER_LAYOUT_UNDEFINED)
                d3dBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COMMON;
            d3dBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_COMMON;
        }
    }

    for(auto &b : textureAcquireBarriers)
    {
        const bool shouldEmit = b.beforeLayout != b.afterLayout;
        const bool shouldEmitOnThisQueue = b.beforeQueue > b.afterQueue;
        if(!shouldEmit || !shouldEmitOnThisQueue)
        {
            continue;
        }

        const Format format = b.texture->GetFormat();

        auto &d3dBarrier = d3dTextureBarriers.emplace_back();
        d3dBarrier.pResource    = static_cast<DirectX12Texture *>(b.texture)->_internalGetNativeTexture().Get();
        d3dBarrier.LayoutBefore = TranslateTextureLayout(b.beforeLayout, format);
        d3dBarrier.LayoutAfter  = TranslateTextureLayout(b.afterLayout, format);
        d3dBarrier.SyncBefore   = TranslateBarrierSync(D3D12_BARRIER_SYNC_ALL, format);
        d3dBarrier.SyncAfter    = TranslateBarrierSync(b.afterStages, format);
        d3dBarrier.AccessBefore = TranslateBarrierAccess(D3D12_BARRIER_ACCESS_COMMON);
        d3dBarrier.AccessAfter  = TranslateBarrierAccess(b.afterAccesses);
        d3dBarrier.Subresources = TranslateBarrierSubresources(b.subresources, b.texture->GetFormat());
        d3dBarrier.Flags        = D3D12_TEXTURE_BARRIER_FLAG_NONE;

        assert(b.afterQueue != QueueType::Transfer);
        if(b.beforeQueue == QueueType::Transfer)
        {
            d3dBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COMMON;
        }

        if(d3dBarrier.LayoutBefore == D3D12_BARRIER_LAYOUT_UNDEFINED)
        {
            d3dBarrier.Flags |= D3D12_TEXTURE_BARRIER_FLAG_DISCARD;
        }

        if(b.texture->GetConcurrentAccessMode() == QueueConcurrentAccessMode::Concurrent)
        {
            if(d3dBarrier.LayoutBefore != D3D12_BARRIER_LAYOUT_UNDEFINED)
                d3dBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COMMON;
            d3dBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_COMMON;
        }
    }

    StaticVector<D3D12_BARRIER_GROUP, 3> barrierGroups;
    if(!d3dGlobalBarriers.empty())
    {
        barrierGroups.PushBack(D3D12_BARRIER_GROUP
        {
            .Type = D3D12_BARRIER_TYPE_GLOBAL,
            .NumBarriers = static_cast<UINT32>(d3dGlobalBarriers.size()),
            .pGlobalBarriers = d3dGlobalBarriers.data()
        });
    }
    if(!d3dTextureBarriers.empty())
    {
        barrierGroups.PushBack(D3D12_BARRIER_GROUP
        {
            .Type = D3D12_BARRIER_TYPE_TEXTURE,
            .NumBarriers = static_cast<UINT32>(d3dTextureBarriers.size()),
            .pTextureBarriers = d3dTextureBarriers.data()
        });
    }
    if(!d3dBufferBarriers.empty())
    {
        barrierGroups.PushBack(D3D12_BARRIER_GROUP
        {
            .Type = D3D12_BARRIER_TYPE_BUFFER,
            .NumBarriers = static_cast<UINT32>(d3dBufferBarriers.size()),
            .pBufferBarriers = d3dBufferBarriers.data()
        });
    }
    if(!barrierGroups.IsEmpty())
    {
        commandList_->Barrier(static_cast<UINT32>(barrierGroups.GetSize()), barrierGroups.GetData());
    }
}

RTRC_RHI_D3D12_END
