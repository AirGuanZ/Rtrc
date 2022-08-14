#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

void Run()
{
    auto instance = CreateVulkanInstance(RHI::VulkanInstanceDesc{
        .debugMode = RTRC_DEBUG
    });

    auto device = instance->CreateDevice(RHI::DeviceDesc{
        .graphicsQueue    = false,
        .computeQueue     = true,
        .transferQueue    = true,
        .supportSwapchain = false
    });

    ShaderManager shaderManager(device);
    shaderManager.SetFileLoader("Asset/02.ComputeShader/");
    auto shader = shaderManager.AddShader({
        .CS = { .filename = "Shader.hlsl", .entry = "CSMain" }
    });

    // create pipeline

    auto bindingGroupLayout = shaderManager.GetBindingGroupLayoutByName("ScaleGroup");

    auto bindingLayout = shader->GetRHIBindingLayout();

    auto pipeline = (*device->CreateComputePipelineBuilder())
        .SetComputeShader(shader->GetRawShader(RHI::ShaderStage::ComputeShader))
        .SetBindingLayout(bindingLayout)
        .CreatePipeline();

    // input texture

    const auto inputImageData = ImageDynamic::Load("Asset/01.TexturedQuad/MainTexture.png");

    auto inputTexture = device->CreateTexture2D(RHI::Texture2DDesc{
        .format       = RHI::Format::B8G8R8A8_UNorm,
        .width        = inputImageData.GetWidth(),
        .height       = inputImageData.GetHeight(),
        .mipLevels    = 1,
        .arraySize    = 1,
        .sampleCount  = 1,
        .usage        = RHI::TextureUsage::TransferDst | RHI::TextureUsage::ShaderResource,
        .initialLayout = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    auto inputTextureSRV = inputTexture->Create2DSRV(RHI::Texture2DSRVDesc{
        .format         = RHI::Format::Unknown,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1
    });

    ResourceUploader uploader(device);

    auto computeQueue = device->GetQueue(RHI::QueueType::Compute);

    std::vector<RHI::TextureAcquireBarrier> pendingTextureAcquireBarriers;
    pendingTextureAcquireBarriers.push_back(uploader.Upload(
        inputTexture, 0, 0,
        inputImageData,
        computeQueue,
        RHI::PipelineStage::ComputeShader,
        RHI::ResourceAccess::TextureRead,
        RHI::TextureLayout::ShaderTexture));

    uploader.SubmitAndSync();

    // output texture

    auto outputTexture = device->CreateTexture2D(RHI::Texture2DDesc{
        .format               = RHI::Format::B8G8R8A8_UNorm,
        .width                = inputImageData.GetWidth(),
        .height               = inputImageData.GetHeight(),
        .mipLevels            = 1,
        .arraySize            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::TransferSrc | RHI::TextureUsage::UnorderAccess,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    auto outputTextureUAV = outputTexture->Create2DUAV(RHI::Texture2DUAVDesc{
        .format         = RHI::Format::Unknown,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1
    });

    // readback staging buffer

    auto readBackStagingBuffer = device->CreateBuffer(RHI::BufferDesc{
        .size                 = static_cast<size_t>(inputImageData.GetWidth() * inputImageData.GetHeight() * 4),
        .usage                = RHI::BufferUsage::TransferDst,
        .hostAccessType       = RHI::BufferHostAccessType::Random,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    // create constant buffer

    auto constantBuffer = device->CreateBuffer(RHI::BufferDesc{
        .size                 = 16,
        .usage                = RHI::BufferUsage::ShaderConstantBuffer,
        .hostAccessType       = RHI::BufferHostAccessType::SequentialWrite,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    constantBuffer->SetName("test");

    {
        const float scaleFactor = 2.0f;
        auto mappedConstantBuffer = constantBuffer->Map(0, 16);
        std::memcpy(mappedConstantBuffer, &scaleFactor, sizeof(scaleFactor));
        constantBuffer->Unmap(0, 16);
    }

    // create binding group

    auto bindingGroup = bindingGroupLayout->CreateBindingGroup();
    bindingGroup->Set("ScaleSetting", constantBuffer, 0, 16);
    bindingGroup->Set("InputTexture", inputTextureSRV);
    bindingGroup->Set("OutputTexture", outputTextureUAV);

    // queue & command pool & buffer

    auto commandPool = computeQueue->CreateCommandPool();
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

    auto mappedOutputBuffer = readBackStagingBuffer->Map(0, readBackStagingBuffer->GetDesc().size);
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
