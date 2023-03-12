#pragma once

#include <semaphore>
#include <thread>

#include <Rtrc/Graphics/ImGui/Instance.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/Scene/Camera/Camera.h>
#include <Rtrc/Scene/Scene.h>
#include <Rtrc/Utility/Timer.h>

RTRC_BEGIN

class RenderLoop : public Uncopyable
{
public:

    struct Command_ResizeFramebuffer
    {
        uint32_t               width;
        uint32_t               height;
        std::binary_semaphore *finishSemaphore;
    };

    struct Command_RenderFrame
    {
        Box<Camera>            cameraProxy;
        Box<SceneProxy>        sceneProxy;
        Box<ImGuiDrawData>     imguiDrawData;
        std::binary_semaphore *finishSemaphore;
    };

    struct Command_Exit
    {
        
    };

    using Command = Variant<Command_ResizeFramebuffer, Command_RenderFrame, Command_Exit>;

    explicit RenderLoop(Device &device);
    ~RenderLoop();

    void AddCommand(Command command);

private:

    void RenderThreadEntry();

    void RenderSingleFrame(const Command_RenderFrame &frameData);

    Device            &device_;
    Box<ImGuiRenderer> imguiRenderer_;
    Box<RG::Executer>  renderGraphExecuter_;

    std::jthread                   renderThread_;
    tbb::concurrent_queue<Command> renderCommandQueue_;

    RC<Tlas> basicTlas_;

    Timer frameTimer_;
};

RTRC_END
