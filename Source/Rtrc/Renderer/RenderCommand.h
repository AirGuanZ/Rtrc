#pragma once

#include <semaphore>

#include <Rtrc/Graphics/ImGui/Instance.h>
#include <Rtrc/Scene/Camera/Camera.h>
#include <Rtrc/Scene/Scene.h>

RTRC_RENDERER_BEGIN

struct RenderCommand_ResizeFramebuffer
{
    uint32_t               width;
    uint32_t               height;
    std::binary_semaphore *finishSemaphore;
};

struct RenderCommand_RenderStandaloneFrame
{
    Box<SceneProxy>        scene;
    RenderCamera           camera;
    Box<ImGuiDrawData>     imguiDrawData;
    RC<BindingGroup>       bindlessTextureGroup;
    std::binary_semaphore *finishSemaphore;
};

struct RenderCommand_Exit
{
    
};

using RenderCommand = Variant<
    RenderCommand_ResizeFramebuffer,
    RenderCommand_RenderStandaloneFrame,
    RenderCommand_Exit>;

RTRC_RENDERER_END
