#include <Rtrc/Renderer/GPUScene/RenderCamera.h>
#include <Rtrc/Renderer/VXIR/Shader/VXIR.shader.outh>
#include <Rtrc/Renderer/VXIR/VXIR.h>

RTRC_RENDERER_BEGIN

void VXIR::Render(ObserverPtr<RG::RenderGraph> renderGraph, ObserverPtr<RenderCamera> renderCamera) const
{
    RTRC_RG_SCOPED_PASS_GROUP(renderGraph, "VXIR");

    auto &data = renderCamera->GetVXIRData();

    PrepareHashTable(renderGraph, data);

    auto hashTableKeys = renderGraph->RegisterBuffer(data.hashTableKeys);
    auto hashTableValues = renderGraph->RegisterBuffer(data.hashTableValues);

    // Update existing hash table items

    {
        
    }
}

void VXIR::ClearFrameData(PerCameraData &data) const
{
    
}

void VXIR::PrepareHashTable(ObserverPtr<RG::RenderGraph> renderGraph, PerCameraData &data) const
{
    using namespace VXIRDetail;

    if(data.hashTableKeys && data.hashTableKeys->GetSize() == settings_.hashTableSize * sizeof(HashTableKey))
    {
        return;
    }

    data.hashTableKeys = device_->CreatePooledBuffer(RHI::BufferDesc
    {
        .size = settings_.hashTableSize * sizeof(HashTableKey),
        .usage = RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::ShaderRWStructuredBuffer
    });
    data.hashTableKeys->SetName("VXIR HashTable Keys");
    data.hashTableKeys->SetDefaultStructStride(sizeof(HashTableKey));

    data.hashTableKeys = device_->CreatePooledBuffer(RHI::BufferDesc
    {
        .size = settings_.hashTableSize * sizeof(HashTableValue),
        .usage = RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::ShaderRWStructuredBuffer
    });
    data.hashTableKeys->SetName("VXIR HashTable Keys");
    data.hashTableKeys->SetDefaultStructStride(sizeof(HashTableValue));

    data.hashTableValues = device_->CreatePooledBuffer(RHI::BufferDesc
    {
        .size = settings_.hashTableSize * sizeof(HashTableValue),
        .usage = RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::ShaderRWStructuredBuffer
    });
    data.hashTableValues->SetName("VXIR HashTable Values");
    data.hashTableValues->SetDefaultStructStride(sizeof(HashTableValue));
    
    auto shader = GetStaticShader<"VXIR/ClearHashTable">();
    StaticShaderInfo<"VXIR/ClearHashTable">::Pass passData;
    passData.HashTableKeyBuffer = renderGraph->RegisterBuffer(data.hashTableKeys);
    passData.HashTableValueBuffer = renderGraph->RegisterBuffer(data.hashTableValues);
    passData.hashTableSize = settings_.hashTableSize;
    renderGraph->CreateComputePassWithThreadCount("ClearHashTable", shader, settings_.hashTableSize, passData);
}

RTRC_RENDERER_END
