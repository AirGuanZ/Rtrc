#pragma once

#include <semaphore>

#include <Rtrc/Renderer/Scene/Camera/Camera.h>
#include <Rtrc/Utility/Variant.h>

RTRC_BEGIN

class SceneProxy;

struct ResizeFramebuffer
{
    uint32_t width;
    uint32_t height;
    std::binary_semaphore *semaphore;
};

struct TryRenderFrame
{
    Camera cameraProxy;
    Box<SceneProxy> sceneProxy;
    std::binary_semaphore *finishRenderSemaphore;
};

struct ExitRenderLoop
{
    
};

using RenderCommand = Variant<ResizeFramebuffer, TryRenderFrame, ExitRenderLoop>;

RTRC_END
