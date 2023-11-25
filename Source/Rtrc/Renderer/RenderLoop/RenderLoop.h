#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_BEGIN

struct ImGuiDrawData;

RTRC_END

RTRC_RENDERER_BEGIN

class RenderLoop
{
public:
    
    struct FrameInput
    {
        const Scene         *scene;
        const Camera        *camera;
        const ImGuiDrawData *imguiDrawData;
    };

    virtual ~RenderLoop() = default;

    virtual void BeginRenderLoop() = 0;
    virtual void EndRenderLoop() = 0;

    virtual void ResizeFramebuffer(uint32_t width, uint32_t height) = 0;
    virtual void RenderFrame(const FrameInput &frame) = 0;
};

RTRC_RENDERER_END
