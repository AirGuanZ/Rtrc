#include <magic_enum.hpp>

#include <Rtrc/Core/Resource/Image.h>
#include <Rtrc/Core/Timer.h>
#include <Rtrc/Graphics/Shader/Keyword.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/ToolKit/DFDM/DFDM.h>

#include "DFDM.shader.outh"

RTRC_BEGIN
    Image<Vector2f> DFDM::GenerateCorrectionMap(const Image<Vector3f> &displacementMapData) const
{
    if(!device_)
    {
        LogError("Rtrc::DFDM::GenerateCorrectionMap: device is not set");
        return {};
    }

    LogInfo("Rtrc::DFDM: distortion-free displacement mapping optimizer");
    LogInfo("   Input displacement map resolution: {}x{}", displacementMapData.GetWidth(), displacementMapData.GetHeight());
    LogInfo("   Wrap mode:                         {}",    magic_enum::enum_name(wrapMode_));
    LogInfo("   Number of iterations:              {}",    iterationCount_);

    Timer timer;
    timer.Restart();

    // Initialize resources

    auto displacementMap = StatefulTexture::FromTexture(device_->CreateAndUploadTexture2D(
        RHI::TextureDesc
        {
            .dim = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::R32G32B32_Float,
            .width = displacementMapData.GetWidth(),
            .height = displacementMapData.GetHeight(),
            .usage = RHI::TextureUsage::ShaderResource,
            .linearHint = true,
        },
        ImageDynamic(displacementMapData),
        RHI::TextureLayout::ShaderTexture));
    displacementMap->SetState(TextureSubrscState(
        RHI::TextureLayout::ShaderTexture,
        RHI::PipelineStage::None,
        RHI::ResourceAccess::None));
    displacementMap->SetName("DisplacementMap");

    auto correctionMap = device_->CreateStatefulTexture(RHI::TextureDesc
    {
        .format = RHI::Format::R32G32_Float,
        .width = displacementMap->GetWidth(),
        .height = displacementMap->GetHeight(),
        .usage = RHI::TextureUsage::TransferDst | RHI::TextureUsage::TransferSrc
    });
    correctionMap->SetName("FinalCorrectionMap");

    // Build & execute render graph

    RG::Executer graphExecuter(device_);
    auto graph = device_->CreateRenderGraph();

    auto input = graph->RegisterTexture(displacementMap);
    auto correctionA = graph->CreateTexture(RHI::TextureDesc
    {
        .format = RHI::Format::R32G32_Float,
        .width  = input->GetWidth(),
        .height = input->GetHeight(),
        .usage  = RHI::TextureUsage::UnorderAccess | RHI::TextureUsage::ShaderResource | RHI::TextureUsage::TransferSrc
    });
    auto correctionB = graph->CreateTexture(correctionA->GetDesc());
    auto rgCorrectionMap = graph->RegisterTexture(correctionMap);
    
    graph->CreateClearRWTexture2DPass("Clear correction texture", correctionA, Vector4f(0, 0, 0, 0));

    for(int i = 0; i < iterationCount_; ++i)
    {
        FastKeywordContext keywords;
        keywords.Set(RTRC_FAST_KEYWORD(WRAP_MODE), static_cast<int>(wrapMode_));

        using Shader = RtrcShader::Builtin::DFDM;
        auto shader = device_->GetShaderTemplate<Shader::Name>()->GetVariant(keywords);

        Shader::Variant<Shader::WRAP_MODE::CLAMP>::Pass passData;
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

    graph->CreateCopyColorTexturePass("CopyToFinalCorrectionMap", correctionA, rgCorrectionMap);

    graphExecuter.Execute(graph);

    // Read back result

    Image<Vector2f> result(input->GetWidth(), input->GetHeight());
    device_->Download(correctionMap, { 0, 0 }, result.GetData());

    timer.BeginFrame();
    LogInfo("Finished in {}s", timer.GetDeltaSecondsF());

    return result;
}

RTRC_END
