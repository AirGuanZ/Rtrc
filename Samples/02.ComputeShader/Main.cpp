#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

$struct_begin(ScaleSetting)
    $variable(float, Factor)
$struct_end()

$group_begin(ScaleGroup)
    $binding(ConstantBuffer<ScaleSetting>, ScaleSettingCBuffer, CS)
    $binding(Texture2D<float4>,            InputTexture,        CS)
    $binding(RWTexture2D<float4>,          OutputTexture,       CS)
$group_end()

void Run()
{
    auto instance = CreateVulkanInstance(RHI::VulkanInstanceDesc{
        .debugMode = true
    });

    auto device = instance->CreateDevice(RHI::DeviceDesc{
        .graphicsQueue    = false,
        .computeQueue     = true,
        .transferQueue    = true,
        .supportSwapchain = false
    });

    // create pipeline

    const std::string shaderSource = File::ReadTextFile("Asset/02.ComputeShader/Shader.hlsl");

    auto computeShader = ShaderCompiler()
        .SetComputeShaderSource(shaderSource, "CSMain")
        .AddBindingGroup<ScaleGroup>()
        .SetDebugMode(true)
        .SetTarget(ShaderCompiler::Target::Vulkan)
        .Compile(*device)->GetComputeShader();

    auto bindingGroupLayout = device->CreateBindingGroupLayout(GetBindingGroupLayoutDesc<ScaleGroup>());
    auto bindingLayout = device->CreateBindingLayout(RHI::BindingLayoutDesc{
        .groups = { bindingGroupLayout }
    });

    auto pipeline = (*device->CreateComputePipelineBuilder())
        .SetComputeShader(computeShader)
        .SetBindingLayout(bindingLayout)
        .CreatePipeline();

    // input texture

    const auto inputImageData = ImageDynamic::Load("Asset/02.ComputeShader/InputTexture.png");

    auto inputTexture = device->CreateTexture2D(RHI::Texture2DDesc{
        .format       = RHI::Format::B8G8R8A8_UNorm,
        .width        = inputImageData.GetWidth(),
        .height       = inputImageData.GetHeight(),
        .mipLevels    = 1,
        .arraySize    = 1,
        .sampleCount  = 1,
        .usage        = RHI::TextureUsage::TransferDst | RHI::TextureUsage::ShaderResource,
        .initialState = RHI::ResourceState::Uninitialized,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    auto inputTextureSRV = inputTexture->Create2DSRV(RHI::Texture2DSRVDesc{
        .format         = RHI::Format::Unknown,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1
    });

    RHI::ResourceUploader uploader(device);

    std::vector<RHI::TextureAcquireBarrier> pendingTextureAcquireBarriers;
    pendingTextureAcquireBarriers.push_back(uploader.Upload(
        inputTexture,  RHI::AspectType::Color, 0, 0,
        inputImageData,
        device->GetQueue(RHI::QueueType::Compute),
        RHI::ResourceState::TextureRead | RHI::ResourceState::CS));

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
        .initialState         = RHI::ResourceState::Uninitialized,
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
        .size                 = inputImageData.GetWidth() * inputImageData.GetHeight() * 4,
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

    {
        const float scaleFactor = 2.0f;
        auto mappedConstantBuffer = constantBuffer->Map(0, 16);
        std::memcpy(mappedConstantBuffer, &scaleFactor, sizeof(scaleFactor));
        constantBuffer->Unmap(0, 16);
    }

    // create binding group

    auto bindingGroup = bindingGroupLayout->CreateBindingGroup();

    ModifyBindingGroup(bindingGroup.get(), &ScaleGroup::ScaleSettingCBuffer, constantBuffer, 0, 16);
    ModifyBindingGroup(bindingGroup.get(), &ScaleGroup::InputTexture, inputTextureSRV);
    ModifyBindingGroup(bindingGroup.get(), &ScaleGroup::OutputTexture, outputTextureUAV);

    // queue & command pool & buffer

    auto computeQueue = device->GetQueue(RHI::QueueType::Compute);

    auto commandPool = computeQueue->CreateCommandPool();
    auto commandBuffer = commandPool->NewCommandBuffer();

    // record

    commandBuffer->Begin();

    const RHI::TextureTransitionBarrier outputTextureInitialStateTransitionBarrier = {
        .texture        = outputTexture,
        .aspectTypeFlag = RHI::AspectType::Color,
        .mipLevel       = 0,
        .arrayLayer     = 0,
        .beforeState    = RHI::ResourceState::Uninitialized,
        .afterState     = RHI::ResourceState::RWTextureWrite | RHI::ResourceState::CS
    };

    commandBuffer->ExecuteBarriers(
        outputTextureInitialStateTransitionBarrier, {}, {}, pendingTextureAcquireBarriers, {}, {});

    commandBuffer->BindPipeline(pipeline);
    commandBuffer->BindGroupToComputePipeline(0, bindingGroup);

    constexpr Vector2i GROUP_SIZE = Vector2i(8, 8);
    const Vector2i groupCount = {
        (static_cast<int>(inputImageData.GetWidth()) + GROUP_SIZE.x - 1) / GROUP_SIZE.x,
        (static_cast<int>(inputImageData.GetHeight()) + GROUP_SIZE.y - 1) / GROUP_SIZE.y
    };
    commandBuffer->Dispatch(groupCount.x, groupCount.y, 1);

    commandBuffer->ExecuteBarriers(
        RHI::TextureTransitionBarrier{
            .texture        = outputTexture,
            .aspectTypeFlag = RHI::AspectType::Color,
            .mipLevel       = 0,
            .arrayLayer     = 0,
            .beforeState    = RHI::ResourceState::RWTextureWrite | RHI::ResourceState::CS,
            .afterState     = RHI::ResourceState::CopySrc
        }, {}, {}, {}, {}, {});

    commandBuffer->CopyTextureToBuffer(readBackStagingBuffer, 0, outputTexture, RHI::AspectType::Color, 0, 0);

    commandBuffer->End();

    // submit

    computeQueue->Submit({}, {}, commandBuffer, {}, {}, {});

    // readback

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

    outputImageData.Save("02.ComputeShader_Output.png");
}

int main()
{
    try
    {
        Run();
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
