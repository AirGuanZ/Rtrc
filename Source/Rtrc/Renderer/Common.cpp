#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

PointLightShadingData ExtractPointLightShadingData(const Light::SharedRenderingData *light)
{
    PointLightShadingData data;
    data.position = light->GetPosition();
    data.color = light->GetIntensity() * light->GetColor();
    // distFade = saturate((distEnd - dist) / (distEnd - distBegin))
    //          = saturate(-dist/(distEnd-distBegin) + distEnd/(distEnd-distBegin))
    const float deltaDist = (std::max)(light->GetDistanceFadeEnd() - light->GetDistanceFadeBegin(), 1e-5f);
    data.distFadeScale = -1.0f / deltaDist;
    data.distFadeBias = light->GetDistanceFadeEnd() / deltaDist;
    return data;
}

DirectionalLightShadingData ExtractDirectionalLightShadingData(const Light::SharedRenderingData *light)
{
    DirectionalLightShadingData data;
    data.direction = light->GetDirection();
    data.color = light->GetIntensity() * light->GetColor();
    return data;
}

RTRC_RENDERER_END
