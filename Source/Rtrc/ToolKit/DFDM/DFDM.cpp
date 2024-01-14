#include <magic_enum.hpp>

#include <Rtrc/Core/Resource/Image.h>
#include <Rtrc/Graphics/Shader/Keyword.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/ToolKit/DFDM/DFDM.h>

#include "DFDM.shader.outh"

RTRC_BEGIN

Image<Vector2f> DFDM::GenerateCorrectionMap(const Image<Vector3f> &displacementMap) const
{
    if(!resources_)
    {
        LogError("Rtrc::DFDM::GenerateCorrectionMap: resources manager is not set");
        return {};
    }

    LogInfo("Rtrc::DFDM: distortion-free displacement mapping optimizer");
    LogInfo("   Input displacement map resolution: {}x{}", displacementMap.GetWidth(), displacementMap.GetHeight());
    LogInfo("   Wrap mode:                         {}",    magic_enum::enum_name(wrapMode_));
    LogInfo("   Number of iterations:              {}",    iterationCount_);

    auto device = resources_->GetDevice();

    // Initialize resources

    auto displacementMapTexture = StatefulTexture::FromTexture(device->CreateAndUploadTexture2D(
        RHI::TextureDesc
        {
            .dim = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::R32G32B32_Float,
            .width = displacementMap.GetWidth(),
            .height = displacementMap.GetHeight(),
            .usage = RHI::TextureUsage::ShaderResource,
            .linearHint = true,
        },
        ImageDynamic(displacementMap),
        RHI::TextureLayout::ShaderTexture));
    displacementMapTexture->SetState(TextureSubrscState(
        RHI::TextureLayout::ShaderTexture,
        RHI::PipelineStage::None,
        RHI::ResourceAccess::None));
    displacementMapTexture->SetName("DisplacementMap");

    const size_t rowAlignment = device->GetTextureBufferCopyRowPitchAlignment(RHI::Format::R32G32_Float);
    const size_t rowSize = UpAlignTo(sizeof(Vector2f) * displacementMap.GetWidth(), rowAlignment);
    const size_t bufferSize = rowSize * displacementMap.GetHeight();

    auto readBackBuffer = device->CreateBuffer(RHI::BufferDesc
    {
        .size = bufferSize,
        .usage = RHI::BufferUsage::TransferDst,
        .hostAccessType = RHI::BufferHostAccessType::Readback
    });

    // Build & execute render graph

    RG::Executer graphExecuter(device);
    auto graph = device->CreateRenderGraph();

    auto input = graph->RegisterTexture(displacementMapTexture);
    auto correctionA = graph->CreateTexture(RHI::TextureDesc
    {
        .format = RHI::Format::R32G32_Float,
        .width  = input->GetWidth(),
        .height = input->GetHeight(),
        .usage  = RHI::TextureUsage::UnorderAccess | RHI::TextureUsage::ShaderResource | RHI::TextureUsage::TransferSrc
    });
    auto correctionB = graph->CreateTexture(correctionA->GetDesc());
    auto rgReadbackBuffer = graph->RegisterBuffer(StatefulBuffer::FromBuffer(readBackBuffer));

    graph->CreateClearRWTexture2DPass("Clear correction texture", correctionA, Vector4f(0, 0, 0, 0));

    for(int i = 0; i < iterationCount_; ++i)
    {
        FastKeywordContext keywords;
        keywords.Set(RTRC_FAST_KEYWORD(WRAP_MODE), static_cast<int>(wrapMode_));
        auto shader = resources_->GetStaticShaderTemplate<"Rtrc/Builtin/DFDM">()->GetVariant(keywords);

        using ShaderInfo = StaticShaderInfo<"Rtrc/Builtin/DFDM">;
        ShaderInfo::Variant<ShaderInfo::WRAP_MODE::CLAMP>::Pass passData;
        passData.DisplacementMap  = input;
        passData.OldCorrectionMap = correctionA;
        passData.NewCorrectionMap = correctionB;

        passData.resolution       = input->GetSize().To<int>();
        passData.areaPreservation = areaPreservation_;
        passData.iteration        = i;

        graph->CreateComputePassWithThreadCount(
            fmt::format("Optimize iteration {}", i),
            shader, input->GetSize(), passData);

        std::swap(correctionA, correctionB);
    }

    auto copyToReadbackBuffer = graph->CreatePass("Copy result to readback buffer");
    copyToReadbackBuffer->Use(correctionA, RG::CopySrc);
    copyToReadbackBuffer->Use(rgReadbackBuffer, RG::CopyDst);
    copyToReadbackBuffer->SetCallback([&]
    {
        RG::GetCurrentCommandBuffer().CopyColorTexture2DToBuffer(
            *rgReadbackBuffer->Get(),
            0, rowSize,
            *correctionA->Get(), 0, 0);
    });

    graphExecuter.Execute(graph);
    device->WaitIdle();

    // Read back result

    std::vector<unsigned char> resultData(bufferSize);
    readBackBuffer->Download(resultData.data(), 0, readBackBuffer->GetSize());

    Image<Vector2f> result(input->GetWidth(), input->GetHeight());
    for(uint32_t y = 0; y < result.GetHeight(); ++y)
    {
        const unsigned char *src = resultData.data() + y * rowSize;
        void *dst = result.GetData() + y * result.GetWidth();
        std::memcpy(dst, src, sizeof(Vector2f) * result.GetWidth());
    }

    LogInfo("Finished");

    return result;
}

RTRC_END
