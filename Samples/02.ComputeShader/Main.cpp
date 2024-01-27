#include <Rtrc/Rtrc.h>

using namespace Rtrc;

#include "Compute.shader.outh"

void Run()
{
    auto device = Device::CreateComputeDevice();

    ResourceManager resourceManager(device);
    
    auto shader = device->GetShader("Compute", true);
    auto pipeline = shader->GetComputePipeline();

    auto inputTexture = device->LoadTexture2D(
        "Asset/Sample/01.TexturedQuad/MainTexture.png", RHI::Format::B8G8R8A8_UNorm, 
        RHI::TextureUsage::ShaderResource, false, RHI::TextureLayout::ShaderTexture);
    auto inputTextureSrv = inputTexture->GetSrv();

    auto outputTexture = device->CreateTexture(RHI::TextureDesc
        {
            .dim = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::B8G8R8A8_UNorm,
            .width = inputTexture->GetDesc().width,
            .height = inputTexture->GetDesc().height,
            .arraySize = 1,
            .mipLevels = 1,
            .sampleCount = 1,
            .usage = RHI::TextureUsage::TransferSrc | RHI::TextureUsage::UnorderAccess,
            .initialLayout = RHI::TextureLayout::Undefined,
            .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
        });
    auto outputTextureUav = outputTexture->GetUav();

    const size_t rowDataSize = outputTexture->GetDesc().width * GetTexelSize(outputTexture->GetFormat());
    const size_t rowDataAlignment = device->GetTextureBufferCopyRowPitchAlignment(outputTexture->GetFormat());
    const size_t alignedRowSize = UpAlignTo(rowDataSize, rowDataAlignment);
    const size_t readbackBufferSize = alignedRowSize * outputTexture->GetDesc().height;
    auto readbackBuffer = device->CreateBuffer(RHI::BufferDesc
        {
            .size = readbackBufferSize,
            .usage = RHI::BufferUsage::TransferDst,
            .hostAccessType = RHI::BufferHostAccessType::Readback
        });

    const int bindingGroupIndex = shader->GetBindingGroupIndexByName("MainGroup");

    StaticShaderInfo<"Compute">::Variant::MainGroup bindingGroupValue;
    bindingGroupValue.InputTexture  = inputTexture;
    bindingGroupValue.OutputTexture = outputTexture;
    bindingGroupValue.scale         = 2.0f;
    auto bindingGroup = device->CreateBindingGroup(bindingGroupValue);

    device->ExecuteAndWait([&](CommandBuffer &cmd)
    {
        cmd.ExecuteBarrier(
            outputTexture,
            RHI::TextureLayout::Undefined,
            RHI::PipelineStage::None,
            RHI::ResourceAccess::None,
            RHI::TextureLayout::ShaderRWTexture,
            RHI::PipelineStage::ComputeShader,
            RHI::ResourceAccess::RWTextureWrite);

        cmd.BindComputePipeline(pipeline);
        cmd.BindComputeGroup(bindingGroupIndex, bindingGroup);

        constexpr Vector2i GROUP_SIZE = Vector2i(8, 8);
        const Vector2i groupCount = {
            (static_cast<int>(inputTexture->GetDesc().width) + GROUP_SIZE.x - 1) / GROUP_SIZE.x,
            (static_cast<int>(inputTexture->GetDesc().height) + GROUP_SIZE.y - 1) / GROUP_SIZE.y
        };
        cmd.Dispatch(groupCount.x, groupCount.y, 1);

        cmd.ExecuteBarrier(
            outputTexture,
            RHI::TextureLayout::ShaderRWTexture,
            RHI::PipelineStage::ComputeShader,
            RHI::ResourceAccess::RWTextureWrite,
            RHI::TextureLayout::CopySrc,
            RHI::PipelineStage::Copy,
            RHI::ResourceAccess::CopyRead);
        cmd.CopyColorTexture2DToBuffer(*readbackBuffer, 0, alignedRowSize, *outputTexture, 0, 0);
    });

    std::vector<unsigned char> readbackData(readbackBufferSize);
    readbackBuffer->Download(readbackData.data(), 0, readbackData.size());

    auto outputImageData = Image<Vector4b>::FromRawData(
        readbackData.data(), outputTexture->GetWidth(), outputTexture->GetHeight(), alignedRowSize);
    for(uint32_t y = 0; y < outputImageData.GetHeight(); ++y)
    {
        for(uint32_t x = 0; x < outputImageData.GetWidth(); ++x)
        {
            auto &p = outputImageData(x, y);
            p = Vector4b(p.z, p.y, p.x, p.w);
        }
    }

    outputImageData.Save("./Asset/Sample/02.ComputeShader/Output.png");
}

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        Run();
    }
    catch(const Exception &e)
    {
        LogError("{}\n{}", e.what(), e.stacktrace());
    }
    catch(const std::exception &e)
    {
        LogError(e.what());
    }
}
