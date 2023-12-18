#include <Rtrc/Renderer/GPUScene/RenderCamera.h>
#include <Rtrc/Renderer/SVGF/SVGF.h>

#include <Rtrc/Renderer/SVGF/Shader/SVGF.shader.outh>

RTRC_RENDERER_BEGIN

void SVGF::Render(
    Ref<RG::RenderGraph> renderGraph,
    Ref<RenderCamera>    renderCamera,
    PerCameraData               &data,
    RG::TextureResource         *inputColor) const
{
    RTRC_RG_SCOPED_PASS_GROUP(renderGraph, "SVGF");

    const uint32_t width = inputColor->GetWidth();
    const uint32_t height = inputColor->GetHeight();
    auto &gbuffers = renderCamera->GetGBuffers();

    // Prepare persistent resources

    std::swap(data.prevColor, data.currColor);
    std::swap(data.prevMoments, data.currMoments);

    if(!data.currColor || data.currColor->GetSize() != inputColor->GetSize())
    {
        data.currColor = device_->CreatePooledTexture(RHI::TextureDesc
        {
            .dim    = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::R32G32B32A32_Float,
            .width  = width,
            .height = height,
            .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
        });
        data.currColor->SetName("SVGF Persistent Color A");

        data.prevColor = device_->CreatePooledTexture(data.currColor->GetDesc());
        data.prevColor->SetName("SVGF Persistent Color B");

        data.currMoments = device_->CreatePooledTexture(RHI::TextureDesc
        {
            .dim    = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::R32G32_Float,
            .width  = width,
            .height = height,
            .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
        });
        data.currMoments->SetName("SVGF Persistent Luminance Moments A");

        data.prevMoments = device_->CreatePooledTexture(data.currMoments->GetDesc());
        data.prevMoments->SetName("SVGF Persistent Luminance Moments B");

        StaticShaderInfo<"SVGF/ClearPrevData">::Pass passData;
        passData.Color      = renderGraph->RegisterTexture(data.prevColor);
        passData.Moments    = renderGraph->RegisterTexture(data.prevMoments);
        passData.resolution = inputColor->GetSize();

        renderGraph->CreateComputePassWithThreadCount(
            "ClearPrevData",
            GetStaticShader<"SVGF/ClearPrevData">(),
            inputColor->GetSize(),
            passData);
    }

    auto prevColor = renderGraph->RegisterTexture(data.prevColor);
    auto currColor = renderGraph->RegisterTexture(data.currColor);

    auto prevMoments = renderGraph->RegisterTexture(data.prevMoments);
    auto currMoments = renderGraph->RegisterTexture(data.currMoments);

    // Temporal filter

    RG::TextureResource *temporalFilterOutputColor, *temporalFilterOutputMoments;
    if(settings_.spatialFilterIterations > 0)
    {
        temporalFilterOutputColor = renderGraph->CreateTexture(RHI::TextureDesc
        {
            .dim    = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::R32G32B32A32_Float,
            .width  = width,
            .height = height,
            .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
        }, "SVGF Temporary Color A");

        temporalFilterOutputMoments = renderGraph->CreateTexture(RHI::TextureDesc
        {
            .dim    = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::R32G32_Float,
            .width  = width,
            .height = height,
            .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
        }, "SVGF Temporary Moments");
    }
    else
    {
        temporalFilterOutputColor = currColor;
        temporalFilterOutputMoments = currMoments;
    }
    
    {
        StaticShaderInfo<"SVGF/TemporalFilter">::Pass passData;
        passData.PrevCamera      = renderCamera->GetPreviousCameraCBuffer();
        passData.CurrCamera      = renderCamera->GetCameraCBuffer();
        passData.PrevDepth       = gbuffers.prevDepth ? gbuffers.prevDepth : gbuffers.currDepth;
        passData.CurrDepth       = gbuffers.currDepth;
        passData.PrevNormal      = gbuffers.prevNormal ? gbuffers.prevNormal : gbuffers.currNormal;
        passData.CurrNormal      = gbuffers.currNormal;
        passData.PrevColor       = prevColor;
        passData.CurrColor       = inputColor;
        passData.ResolvedColor   = temporalFilterOutputColor;
        passData.PrevMoments     = prevMoments;
        passData.ResolvedMoments = temporalFilterOutputMoments;
        passData.resolution      = inputColor->GetSize();
        passData.alpha           = std::clamp(settings_.temporalFilterAlpha, 0.0f, 1.0f);
        
        renderGraph->CreateComputePassWithThreadCount(
            "TemporalFilter",
            GetStaticShader<"SVGF/TemporalFilter">(),
            inputColor->GetSize(),
            passData);
    }

    // Variance estimation

    auto varianceEstimateOutput = renderGraph->CreateTexture(RHI::TextureDesc
    {
        .dim    = RHI::TextureDimension::Tex2D,
        .format = RHI::Format::R32_Float,
        .width  = width,
        .height = height,
        .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
    }, "SVGF Temporary Variance A");

    {
        StaticShaderInfo<"SVGF/VarianceEstimate">::Pass passData;
        passData.Moments    = temporalFilterOutputMoments;
        passData.Variance   = varianceEstimateOutput;
        passData.resolution = inputColor->GetSize();

        renderGraph->CreateComputePassWithThreadCount(
            "VarianceEstimate",
            GetStaticShader<"SVGF/VarianceEstimate">(),
            inputColor->GetSize(),
            passData);
    }

    // Spatial filter

    RG::TextureResource *finalColor = temporalFilterOutputColor;
    if(settings_.spatialFilterIterations > 0)
    {
        auto PrefilterVariance = [&](
            RG::TextureResource *inVariance,
            RG::TextureResource *outVariance)
        {
            StaticShaderInfo<"SVGF/VariancePrefilter">::Pass passData;
            passData.Depth          = gbuffers.currDepth;
            passData.Variance       = inVariance;
            passData.OutputVariance = outVariance;
            passData.resolution     = inVariance->GetSize();

            renderGraph->CreateComputePassWithThreadCount(
                "VariancePrefilter",
                GetStaticShader<"SVGF/VariancePrefilter">(),
                inVariance->GetSize(),
                passData);
        };

        auto variance1 = varianceEstimateOutput;
        auto variance2 = renderGraph->CreateTexture(variance1->GetDesc(), "SVGF Temporary Variance B");
        auto variancef = renderGraph->CreateTexture(variance1->GetDesc(), "SVGF Prefiltered Variance");

        auto color1 = temporalFilterOutputColor;
        auto color2 = renderGraph->CreateTexture(color1->GetDesc(), "SVGF Temporary Color B");
        auto color3 = currColor;
        
        for(unsigned i = 0; i < settings_.spatialFilterIterations; ++i)
        {
            PrefilterVariance(variance1, variancef);

            using ShaderInfo = StaticShaderInfo<"SVGF/SpatialFilter">;

            FastKeywordContext keywords;
            keywords.Set(RTRC_FAST_KEYWORD(IS_DIRECTION_Y), true);

            {
                ShaderInfo::Variant<true>::Pass passData;
                passData.Camera         = renderCamera->GetCameraCBuffer();
                passData.Depth          = gbuffers.currDepth;
                passData.Normal         = gbuffers.currNormal;
                passData.Variance       = variancef;
                passData.Color          = color1;
                passData.OutputColor    = color2;
                passData.OutputVariance = variance2;
                passData.resolution     = inputColor->GetSize();

                renderGraph->CreateComputePassWithThreadCount(
                    "SpatialFilter",
                    GetStaticShader<"SVGF/SpatialFilter">(keywords),
                    inputColor->GetSize(),
                    passData);
            }

            keywords.Set(RTRC_FAST_KEYWORD(IS_DIRECTION_Y), false);
            
            {
                ShaderInfo::Variant<false>::Pass passData;
                passData.Camera         = renderCamera->GetCameraCBuffer();
                passData.Depth          = gbuffers.currDepth;
                passData.Normal         = gbuffers.currNormal;
                passData.Variance       = variance2;
                passData.Color          = color2;
                passData.OutputColor    = color3;
                passData.OutputVariance = variance1;
                passData.resolution     = inputColor->GetSize();

                renderGraph->CreateComputePassWithThreadCount(
                    "SpatialFilter",
                    GetStaticShader<"SVGF/SpatialFilter">(keywords),
                    inputColor->GetSize(),
                    passData);
            }

            finalColor = color3;

            if(i == 0)
            {
                color3 = color1;
                color1 = currColor;
            }
            else if(i == 1)
            {
                color1 = color3;
            }
        }
    }

    assert(!data.finalColor);
    data.finalColor = finalColor;
}

void SVGF::ClearFrameData(PerCameraData &data) const
{
    data.finalColor = nullptr;
}

RTRC_RENDERER_END
