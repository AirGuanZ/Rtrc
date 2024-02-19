#include <Rtrc/Rtrc.h>

using namespace Rtrc;

class GizmoDemo : public SimpleApplication
{
    void InitializeSimpleApplication(GraphRef graph) override
    {
        gizmoRenderer_ = MakeBox<GizmoRenderer>(GetDevice());
        camera_.SetLookAt({ 0, 0, 7 }, { 0, 1, 0 }, { 0, 0, 0 });
        cameraController_.SetCamera(camera_);
        cameraController_.SetTrackballDistance(Length(camera_.GetPosition()));
    }

    void UpdateSimpleApplication(GraphRef renderGraph) override
    {
        if(GetWindowInput().IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        cameraController_.Update(GetWindowInput(), GetFrameTimer().GetDeltaSecondsF());
        camera_.SetAspectRatio(GetWindow().GetFramebufferWOverH());
        camera_.UpdateDerivedData();

        auto framebuffer = renderGraph->RegisterSwapchainTexture(GetDevice()->GetSwapchain());
        auto depthBuffer = renderGraph->CreateTexture(RHI::TextureDesc
        {
            .format = RHI::Format::D24S8,
            .width = framebuffer->GetWidth(),
            .height = framebuffer->GetHeight(),
            .usage = RHI::TextureUsage::DepthStencil,
            .clearValue = RHI::DepthStencilClearValue{ 1, 0 }
        });

        auto clearPass = renderGraph->CreatePass("Clear");
        clearPass->Use(framebuffer, RG::ColorAttachment);
        clearPass->Use(depthBuffer, RG::DepthStencilAttachment);
        clearPass->SetCallback([=]
        {
            auto &cmds = RGGetCommandBuffer();
            cmds.BeginRenderPass(
                RHI::ColorAttachment{
                    .renderTargetView = framebuffer->GetRtvImm(),
                    .loadOp = RHI::AttachmentLoadOp::Clear,
                    .clearValue = { 0, 0, 0, 0 }
                },
                RHI::DepthStencilAttachment
                {
                    .depthStencilView = depthBuffer->GetDsvImm(),
                    .loadOp = RHI::AttachmentLoadOp::Clear,
                    .clearValue = { 1, 0 }
                });
            cmds.EndRenderPass();
        });

        GizmoBuilder gizmo;
        gizmo.DrawLine({ -5, -1, 0 }, { -5, 1, 0 });

        {
            gizmo.PushColor({ 0, 1, 1 });
            gizmo.DrawQuad({ -4, -1, 0 }, { -4, +1, 0 }, { -2, +1, 0 }, { -2, -1, 0 });
            gizmo.PopColor();
        }

        {
            gizmo.PushColor({ 1, 1, 0 });
            gizmo.DrawWireCube({ 0, 0, 0 }, 1);
            gizmo.PopColor();
        }

        {
            gizmo.PushColor({ 1, 0, 1 });
            gizmo.DrawCube({ 3, 0, 0 }, 1);
            gizmo.PopColor();
        }

        gizmoRenderer_->AddRenderPass(
            gizmo, renderGraph, framebuffer, depthBuffer, camera_.GetWorldToClip(), false, 2.2f);
    }

    Camera camera_;
    EditorCameraController cameraController_;
    Box<GizmoRenderer> gizmoRenderer_;
};

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        GizmoDemo().Run(GizmoDemo::Config
        {
            .title             = "Rtrc Sample: Gizmo Rendering",
            .width             = 640,
            .height            = 480,
            .maximized         = true,
            .vsync             = true,
            .debug             = RTRC_DEBUG,
            .rayTracing        = false,
            .backendType       = RHI::BackendType::Default,
            .enableGPUCapturer = false
        });
    }
#if RTRC_ENABLE_EXCEPTION_STACKTRACE
    catch(const Exception &e)
    {
        LogError("{}\n{}", e.what(), e.stacktrace());
    }
#endif
    catch(const std::exception &e)
    {
        LogError(e.what());
    }
}
