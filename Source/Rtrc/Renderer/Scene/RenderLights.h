#pragma once

#include <Rtrc/Renderer/Common.h>
#include <Rtrc/Renderer/Utility/UploadBufferPool.h>

RTRC_RENDERER_BEGIN

// Per-scene light rendering data
class RenderLights
{
public:
    
    explicit RenderLights(ObserverPtr<Device> device);

    void Update(const SceneProxy &scene, RG::RenderGraph &renderGraph);

    void ClearFrameData();

    Span<PointLightShadingData>       GetPointLights()       const;
    Span<DirectionalLightShadingData> GetDirectionalLights() const;

    bool                               HasMainLight()                       const;
    const Light::SharedRenderingData  *GetMainLight()                       const;
    const PointLightShadingData       &GetMainPointLightShadingData()       const;
    const DirectionalLightShadingData &GetMainDirectionalLightShadingData() const;
    
    RG::BufferResource *GetRGPointLightBuffer()       const { return rgPointLightBuffer_; }
    RG::BufferResource* GetRGDirectionalLightBuffer() const { return rgDirectionalLightBuffer_; }

private:

    ObserverPtr<Device> device_;

    std::vector<PointLightShadingData>                                          pointLightShadingData_;
    std::vector<DirectionalLightShadingData>                                    directionalLightShadingData_;
    Variant<std::monostate, PointLightShadingData, DirectionalLightShadingData> mainLightShadingData_;
    const Light::SharedRenderingData                                           *mainLight_ = nullptr;

    UploadBufferPool<> uploadLightBufferPool_;

    RG::BufferResource *rgPointLightBuffer_       = nullptr;
    RG::BufferResource *rgDirectionalLightBuffer_ = nullptr;
};

inline Span<PointLightShadingData> RenderLights::GetPointLights() const
{
    return pointLightShadingData_;
}

inline Span<DirectionalLightShadingData> RenderLights::GetDirectionalLights() const
{
    return directionalLightShadingData_;
}

inline bool RenderLights::HasMainLight() const
{
    return !mainLightShadingData_.Is<std::monostate>();
}

inline const Light::SharedRenderingData *RenderLights::GetMainLight() const
{
    return mainLight_;
}

inline const PointLightShadingData &RenderLights::GetMainPointLightShadingData() const
{
    return mainLightShadingData_.As<PointLightShadingData>();
}

inline const DirectionalLightShadingData &RenderLights::GetMainDirectionalLightShadingData() const
{
    return mainLightShadingData_.As<DirectionalLightShadingData>();
}

RTRC_RENDERER_END
