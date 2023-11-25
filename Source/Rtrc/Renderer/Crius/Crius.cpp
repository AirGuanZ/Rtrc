#include <Rtrc/Renderer/Crius/Crius.h>
#include <Rtrc/Renderer/GPUScene/RenderCamera.h>

RTRC_RENDERER_BEGIN

void Crius::Render(
    ObserverPtr<RG::RenderGraph> renderGraph,
    ObserverPtr<RenderCamera>    renderCamera) const
{
    auto &data = renderCamera->GetCriusData();

    // Prepare buffers

    bool clearCounterBuffer = false;
    if(!data.surfelBuffer || data.surfelBuffer->GetSize() < sizeof(CriusDetail::Surfel) * settings_.maxSurfelCount)
    {
        clearCounterBuffer = true;

        data.surfelBuffer = device_->CreatePooledBuffer(RHI::BufferDesc
        {
            .size = sizeof(CriusDetail::Surfel) * settings_.maxSurfelCount,
            .usage = RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::ShaderRWStructuredBuffer
        }, "CriusSurfelBuffer");

        if(!data.surfelCounterBuffer)
        {
            data.surfelCounterBuffer = device_->CreatePooledBuffer(RHI::BufferDesc
            {
                .size = sizeof(uint32_t)  * CriusDetail::SC_Count,
                .usage = RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::ShaderRWStructuredBuffer
            }, "CriusSurfelCounterBuffer");
        }
    }

    auto surfelBuffer = renderGraph->RegisterBuffer(data.surfelBuffer);
    auto surfelCounterBuffer = renderGraph->RegisterBuffer(data.surfelCounterBuffer);

    if(clearCounterBuffer)
    {
        renderGraph->CreateClearRWStructuredBufferPass("ClearSurfelCounterBuffer", surfelCounterBuffer, 0);
    }

    // Build acceleration structure



    // TODO
}

RTRC_RENDERER_END
