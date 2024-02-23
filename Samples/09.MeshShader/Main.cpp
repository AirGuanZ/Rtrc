#include <Rtrc/Rtrc.h>

#include "UniformGrid.shader.outh"

using namespace Rtrc;

class MeshShaderDemo : public SimpleApplication
{
    void InitializeSimpleApplication(GraphRef graph) override
    {
        pipelineCache_.SetDevice(GetDevice());
    }

    void UpdateSimpleApplication(GraphRef graph) override
    {
        auto &input = GetWindowInput();
        if(input.IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        auto swapchainTexture = graph->RegisterSwapchainTexture(GetDevice()->GetSwapchain());
        
        RGAddRenderPass<true>(graph, "RenderUniformGrid", RGColorAttachment
        {
            .rtv = swapchainTexture->GetRtv(),
            .loadOp = RHI::AttachmentLoadOp::Clear,
            .clearValue = { 0, 0, 0, 0 }
        }, [renderTarget = swapchainTexture, this]
        {
            const Vector2f viewportSize = renderTarget->GetSize().To<float>();
            Vector2f clipRectSize;
            if(viewportSize.x >= viewportSize.y)
            {
                clipRectSize.y = 1.6f;
                clipRectSize.x = clipRectSize.y * viewportSize.y / viewportSize.x;
            }
            else
            {
                clipRectSize.x = 1.6f;
                clipRectSize.y = clipRectSize.x * viewportSize.x / viewportSize.y;
            }

            using Shader = RtrcShader::MeshShader::UniformGrid;
            Shader::Pass passData;
            passData.clipLower = -0.5f * clipRectSize;
            passData.clipUpper = +0.5f * clipRectSize;
            auto passGroup = GetDevice()->CreateBindingGroupWithCachedLayout(passData);

            auto pipeline = pipelineCache_.Get(
            {
                .shader = GetDevice()->GetShader<Shader::Name>(),
                .rasterizerState = RTRC_STATIC_RASTERIZER_STATE(
                {
                    .fillMode = RHI::FillMode::Line,
                }),
                .attachmentState = RTRC_ATTACHMENT_STATE
                {
                    .colorAttachmentFormats = { renderTarget->GetFormat() }
                }
            });

            CommandBuffer &commandBuffer= RGGetCommandBuffer();
            commandBuffer.BindGraphicsPipeline(pipeline);
            commandBuffer.BindGraphicsGroups(passGroup);
            commandBuffer.DispatchMesh(1, 1, 1);
        });
    }

    GraphicsPipelineCache pipelineCache_;
};

RTRC_APPLICATION_MAIN(
    MeshShaderDemo,
    .title             = "Rtrc Sample: Mesh Shader",
    .width             = 640,
    .height            = 480,
    .maximized         = true,
    .vsync             = true,
    .debug             = RTRC_DEBUG,
    .rayTracing        = false,
    .backendType       = RHI::BackendType::Default,
    .enableGPUCapturer = false)
