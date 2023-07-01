#include <Rtrc/Renderer/Scene/RenderLights.h>

RTRC_RENDERER_BEGIN

RenderLights::RenderLights(ObserverPtr<Device> device)
    : device_(device), uploadLightBufferPool_(device, RHI::BufferUsage::TransferSrc)
{
    
}

void RenderLights::Update(const SceneProxy &scene, RG::RenderGraph &renderGraph)
{
    pointLightShadingData_.clear();
    directionalLightShadingData_.clear();
    mainLightShadingData_ = std::monostate();

    for(const Light::SharedRenderingData *light : scene.GetLights())
    {
        if(light->GetType() != Light::Type::Point && light->GetType() != Light::Type::Directional)
        {
            // Skip main light of unsupported type
            LogWarning(
                "RenderLights: unsupported light type ({}). "
                "Only point/directional lights are supported",
                std::to_underlying(light->GetType()));
            continue;
        }

        if(light->GetFlags().Contains(Light::Flags::EnableRayTracedShadow))
        {
            if(!mainLightShadingData_.Is<std::monostate>())
            {
                LogWarning("At most one main light is supported. Others are ignored");
                continue;
            }

            mainLight_ = light;
            if(light->GetType() == Light::Type::Point)
            {
                mainLightShadingData_ = ExtractPointLightShadingData(light);
            }
            else
            {
                assert(light->GetType() == Light::Type::Directional);
                mainLightShadingData_ = ExtractDirectionalLightShadingData(light);
            }
        }
        else
        {
            if(light->GetType() == Light::Type::Point)
            {
                pointLightShadingData_.push_back(ExtractPointLightShadingData(light));
            }
            else
            {
                assert(light->GetType() == Light::Type::Directional);
                directionalLightShadingData_.push_back(ExtractDirectionalLightShadingData(light));
            }
        }
    }

    auto UploadLightBuffer = [&]<typename D>(std::string_view bufferName, Span<D> data)
    {
        const size_t requiredBufferSize = std::max(data.GetSize(), 1u) * sizeof(D);
        auto buffer = renderGraph.CreateBuffer(RHI::BufferDesc
        {
            .size  = requiredBufferSize,
            .usage = RHI::BufferUsage::TransferDst | RHI::BufferUsage::ShaderStructuredBuffer
        }, std::string(bufferName));
        if(!data.IsEmpty())
        {
            auto uploadBuffer = uploadLightBufferPool_.Acquire(requiredBufferSize);
            uploadBuffer->SetName(fmt::format("Staging buffer for uploading data to {}", bufferName));
            uploadBuffer->Upload(data.GetData(), 0, requiredBufferSize);

            auto rgUploadBuffer = renderGraph.RegisterBuffer(StatefulBuffer::FromBuffer(std::move(uploadBuffer)));
            renderGraph.CreateCopyBufferPass(
                fmt::format("Copy to {}", bufferName), rgUploadBuffer, buffer, requiredBufferSize);
        }
        return buffer;
    };

    assert(!rgPointLightBuffer_);
    assert(!rgDirectionalLightBuffer_);
    rgPointLightBuffer_       = UploadLightBuffer("PointLightBuffer", Span(pointLightShadingData_));
    rgDirectionalLightBuffer_ = UploadLightBuffer("DirectionalLightBuffer", Span(directionalLightShadingData_));
    
}

void RenderLights::ClearFrameData()
{
    rgPointLightBuffer_ = nullptr;
    rgDirectionalLightBuffer_ = nullptr;
    mainLight_ = nullptr;
}

RTRC_RENDERER_END
