#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

cbuffer_begin(X)
    cbuffer_var(float, x)
cbuffer_end()

cbuffer_begin(ScaleSetting)
    cbuffer_var(X[2], xx)
    cbuffer_var(Vector4f, y)
cbuffer_end()

void Run()
{
    auto instance = CreateVulkanInstance(RHI::VulkanInstanceDesc{
        .debugMode = RTRC_DEBUG
    });

    auto device = instance->CreateDevice();

    RenderContext renderContext(device);

    ShaderCompiler shaderCompiler;
    shaderCompiler.SetRenderContext(&renderContext);
    shaderCompiler.SetRootDirectory("Asset/02.ComputeShader/");
    auto shader = shaderCompiler.Compile({ .filename = "Shader.hlsl", .CSEntry = "CSMain" });

    // create pipeline

    auto bindingGroupLayout = shader->GetBindingGroupLayoutByName("ScaleGroup");

    auto bindingLayout = shader->GetRHIBindingLayout();

    auto pipeline = RHI::ComputePipelineBuilder()
        .SetComputeShader(shader->GetRawShader(RHI::ShaderStage::ComputeShader))
        .SetBindingLayout(bindingLayout)
        .CreatePipeline(device);

    // input texture

    const auto inputImageData = ImageDynamic::Load("Asset/01.TexturedQuad/MainTexture.png");

    auto inputTexture = renderContext.CreateTexture2D(
        inputImageData.GetWidth(),
        inputImageData.GetHeight(),
        1, 1, RHI::Format::B8G8R8A8_UNorm,
        RHI::TextureUsage::TransferDst | RHI::TextureUsage::ShaderResource,
        RHI::QueueConcurrentAccessMode::Concurrent, 1, true);

    auto inputTextureSRV = inputTexture->GetSRV();

    ResourceUploader uploader(device);

    auto computeQueue = device->GetQueue(RHI::QueueType::Graphics);

    uploader.Upload(
        inputTexture->GetRHIObject(), 0, 0,
        inputImageData,
        computeQueue,
        RHI::PipelineStage::ComputeShader,
        RHI::ResourceAccess::TextureRead,
        RHI::TextureLayout::ShaderTexture);

    uploader.SubmitAndSync();

    // output texture

    auto outputTexture = renderContext.CreateTexture2D(
        inputImageData.GetWidth(),
        inputImageData.GetHeight(),
        1, 1, RHI::Format::B8G8R8A8_UNorm,
        RHI::TextureUsage::TransferSrc | RHI::TextureUsage::UnorderAccess,
        RHI::QueueConcurrentAccessMode::Concurrent,
        1, true);

    auto outputTextureUAV = outputTexture->GetUAV();

    // readback staging buffer

    auto readBackStagingBuffer = device->CreateBuffer(RHI::BufferDesc{
        .size           = static_cast<size_t>(inputImageData.GetWidth() * inputImageData.GetHeight() * 4),
        .usage          = RHI::BufferUsage::TransferDst,
        .hostAccessType = RHI::BufferHostAccessType::Random
    });

    // create constant buffer

    //ResourceManager resourceManager(device, 1);
    auto constantBuffer = renderContext.CreateConstantBuffer();
    {
        ScaleSetting scaleSetting = {};
        scaleSetting.y.x = 2.0f;
        constantBuffer->SetData(scaleSetting);
    }

    // create binding group

    auto bindingGroup = bindingGroupLayout->CreateBindingGroup();
    bindingGroup->Set("ScaleSetting", constantBuffer);
    bindingGroup->Set("InputTexture", inputTextureSRV);
    bindingGroup->Set("OutputTexture", outputTextureUAV);

    // queue & command pool & buffer

    auto commandPool = device->CreateCommandPool(computeQueue);
    auto commandBuffer = commandPool->NewCommandBuffer();

    // record

    commandBuffer->Begin();

    const RHI::TextureTransitionBarrier outputTextureInitialStateTransitionBarrier = {
        .texture      = outputTexture,
        .subresources = {
            .mipLevel   = 0,
            .arrayLayer = 0
        },
        .beforeStages   = RHI::PipelineStage::None,
        .beforeAccesses = RHI::ResourceAccess::None,
        .beforeLayout   = RHI::TextureLayout::Undefined,
        .afterStages    = RHI::PipelineStage::ComputeShader,
        .afterAccesses  = RHI::ResourceAccess::RWTextureWrite,
        .afterLayout    = RHI::TextureLayout::ShaderRWTexture
    };

    commandBuffer->ExecuteBarriers(outputTextureInitialStateTransitionBarrier, pendingTextureAcquireBarriers);

    commandBuffer->BindPipeline(pipeline);
    commandBuffer->BindGroupToComputePipeline(0, bindingGroup->GetRHIBindingGroup());

    constexpr Vector2i GROUP_SIZE = Vector2i(8, 8);
    const Vector2i groupCount = {
        (static_cast<int>(inputImageData.GetWidth()) + GROUP_SIZE.x - 1) / GROUP_SIZE.x,
        (static_cast<int>(inputImageData.GetHeight()) + GROUP_SIZE.y - 1) / GROUP_SIZE.y
    };
    commandBuffer->Dispatch(groupCount.x, groupCount.y, 1);

    commandBuffer->ExecuteBarriers(
        RHI::TextureTransitionBarrier{
            .texture      = outputTexture,
            .subresources = {
                .mipLevel   = 0,
                .arrayLayer = 0
            },
            .beforeStages   = RHI::PipelineStage::ComputeShader,
            .beforeAccesses = RHI::ResourceAccess::RWTextureWrite,
            .beforeLayout   = RHI::TextureLayout::ShaderRWTexture,
            .afterStages    = RHI::PipelineStage::Copy,
            .afterAccesses  = RHI::ResourceAccess::CopyRead,
            .afterLayout    = RHI::TextureLayout::CopySrc
        });

    commandBuffer->CopyColorTexture2DToBuffer(readBackStagingBuffer, 0, outputTexture, 0, 0);

    commandBuffer->End();

    // submit

    computeQueue->Submit({}, {}, commandBuffer, {}, {}, {});

    // read back

    computeQueue->WaitIdle();

    Image<Vector4b> outputImageData(inputImageData.GetWidth(), inputImageData.GetHeight());

    auto mappedOutputBuffer = readBackStagingBuffer->Map(0, readBackStagingBuffer->GetDesc().size, true);
    std::memcpy(outputImageData.GetData(), mappedOutputBuffer, readBackStagingBuffer->GetDesc().size);
    readBackStagingBuffer->Unmap(0, readBackStagingBuffer->GetDesc().size);

    for(uint32_t y = 0; y < outputImageData.GetHeight(); ++y)
    {
        for(uint32_t x = 0; x < outputImageData.GetWidth(); ++x)
        {
            auto &p = outputImageData(x, y);
            p = Vector4b(p.z, p.y, p.x, p.w);
        }
    }

    // save output image

    outputImageData.Save("./Asset/02.ComputeShader/Output.png");
}

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        Run();
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
