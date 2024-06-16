#include <Rtrc/Rtrc.h>

#include "Shader.shader.outh"

using namespace Rtrc;

class LinearInterpolationPrecisionDemo : public SimpleApplication
{
    RC<Texture> texture_;
    RC<GraphicsPipeline> pipeline_;

    void InitializeSimpleApplication(GraphRef graph) override
    {
        const float texData[2] = { 0, 1 };
        texture_ = device_->CreateAndUploadTexture(RHI::TextureDesc
        {
            .dim    = RHI::TextureDimension::Tex1D,
            .format = RHI::Format::R32_Float,
            .width  = 2,
            .usage  = RHI::TextureUsage::ShaderResource
        }, texData, RHI::TextureLayout::ShaderTexture);
    }

    void UpdateSimpleApplication(GraphRef graph) override
    {
        if(input_->IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        auto framebuffer = graph->RegisterSwapchainTexture(device_->GetSwapchain());

        if(!pipeline_)
        {
            pipeline_ = device_->CreateGraphicsPipeline(
            {
                .shader = device_->GetShader<RtrcShader::Draw::Name>(),
                .rasterizerState = RTRC_STATIC_RASTERIZER_STATE(
                {
                    .primitiveTopology = RHI::PrimitiveTopology::LineList
                }),
                .attachmentState = RTRC_ATTACHMENT_STATE
                {
                    .colorAttachmentFormats = { framebuffer->GetFormat() }
                },
            });
        }

        RGAddRenderPass<true>(
            graph, "Draw", RGColorAttachment
            {
                .rtv         = framebuffer->GetRtv(),
                .loadOp      = RHI::AttachmentLoadOp::Clear,
                .clearValue  = RHI::ColorClearValue{ 0, 0, 0, 0 },
                .isWriteOnly = true
            },
            [this]
            {
                RtrcShader::Draw::Pass passData;
                passData.Texture = texture_;
                auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

                auto &commandBuffer = RGGetCommandBuffer();
                commandBuffer.BindGraphicsPipeline(pipeline_);
                commandBuffer.BindGraphicsGroups(passGroup);
                commandBuffer.Draw(2048, 1, 0, 0);
            });
    }
};

RTRC_APPLICATION_MAIN(
    LinearInterpolationPrecisionDemo,
    .title             = "Rtrc Sample: Linear Interpolation Precision",
    .width             = 640,
    .height            = 480,
    .maximized         = true,
    .vsync             = true,
    .debug             = RTRC_DEBUG,
    .rayTracing        = false,
    .backendType       = RHI::BackendType::Default,
    .enableGPUCapturer = false)
