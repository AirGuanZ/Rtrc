#include <random>

#include <Rtrc/Rtrc.h>

#include "OIT.shader.outh"

using namespace Rtrc;

class MultiLayerOITDemo : public SimpleApplication
{
    struct Vertex
    {
        Vector3f position;
        Vector3f normal;
        Vector2f uv;
    };

    RC<Buffer> vertexBuffer_;
    RC<Texture> colorTexture_;
    RC<Texture> specularTexture_;

    GraphicsPipelineCache pipelineCache_;

    Camera camera_;
    EditorCameraController cameraController_;

    uint32_t layerCount_ = 4;
    bool discardFarest_ = true;

    void InitializeSimpleApplication(GraphRef graph) override
    {
        {
            LogInfo("Load mesh");

            auto mesh = Geo::RawMesh::Load("Asset/Sample/20.MultiLayerOIT/Hair.obj");
            const auto positionIndices = mesh.GetPositionIndices();
            const auto normalIndices = mesh.GetNormalIndices();
            const auto uvIndices = mesh.GetUVIndices();

            std::vector<uint32_t> triangleIndices =
                std::views::iota(0u) |
                std::views::take(positionIndices.size() / 3) |
                std::ranges::to<std::vector<uint32_t>>();

            std::ranges::shuffle(triangleIndices, std::default_random_engine{ 42 });

            std::vector<Vertex> vertexData;
            for(uint32_t wedgeIndex = 0; wedgeIndex < positionIndices.size(); ++wedgeIndex)
            {
                const uint32_t mappedWedgeIndex = 3 * triangleIndices[wedgeIndex / 3] + (wedgeIndex % 3);

                const uint32_t positionIndex = positionIndices[mappedWedgeIndex];
                const uint32_t normalIndex   = normalIndices[mappedWedgeIndex];
                const uint32_t uvIndex       = uvIndices[mappedWedgeIndex];
                vertexData.push_back(
                    Vertex
                    {
                        mesh.GetPositionData()[positionIndex],
                        mesh.GetNormalData()[normalIndex],
                        mesh.GetUVData()[uvIndex]
                    });
            }

            vertexBuffer_ = device_->CreateAndUploadStructuredBuffer(
                RHI::BufferUsageFlag::VertexBuffer, Span(vertexData));
        }

        {
            LogInfo("Load textures");

            colorTexture_ = device_->LoadTexture2D(
                "Asset/Sample/20.MultiLayerOIT/IP_NMIXX_F_HAIR_27_02_shd_baseColor.png",
                RHI::Format::R8G8B8A8_UNorm,
                RHI::TextureUsage::ShaderResource,
                false,
                RHI::TextureLayout::ShaderTexture);

            specularTexture_ = device_->LoadTexture2D(
                "Asset/Sample/20.MultiLayerOIT/IP_NMIXX_F_HAIR_27_01_shd_specularf0.png",
                RHI::Format::R8G8B8A8_UNorm,
                RHI::TextureUsage::ShaderResource,
                false,
                RHI::TextureLayout::ShaderTexture);
        }

        pipelineCache_.SetDevice(device_);

        camera_.SetLookAt({ -4, 0, 0 }, { 0, 1, 0 }, { 0, 0, 0 });
        cameraController_.SetCamera(camera_);
        cameraController_.SetTrackballDistance(Length(camera_.GetPosition()));
    }

    void UpdateSimpleApplication(GraphRef graph) override
    {
        if(input_->IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        if(imgui_->Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            imgui_->Input("Layer count (0 to disable)", &layerCount_);
            imgui_->CheckBox("Discard farest layer when overflow", &discardFarest_);
        }
        imgui_->End();

        camera_.SetAspectRatio(window_->GetFramebufferWOverH());
        if(!imgui_->IsAnyItemActive())
        {
            cameraController_.Update(*input_, GetFrameTimer().GetDeltaSecondsF());
        }
        camera_.UpdateDerivedData();

        auto framebuffer = graph->RegisterSwapchainTexture(GetSwapchain());

        if(layerCount_ == 0)
        {
            RenderRaw(graph, framebuffer);
        }
        else
        {
            RenderOIT(graph, framebuffer);
        }
    }

    void RenderRaw(GraphRef graph, RGTexture framebuffer)
    {
        RGAddRenderPass<true>(
            graph, "RenderHair",
            RGColorAttachment
            {
                .rtv        = framebuffer->GetRtv(),
                .loadOp     = RHI::AttachmentLoadOp::Clear,
                .clearValue = { 0, 1, 1, 0 },
            },
            [this, framebuffer]
            {
                using Shader = RtrcShader::Render;

                auto pipeline = pipelineCache_.Get(
                    GraphicsPipelineDesc
                    {
                        .shader = device_->GetShader<Shader::Name>(),
                        .meshLayout = RTRC_STATIC_MESH_LAYOUT(Buffer(
                            Attribute("POSITION", Float3),
                            Attribute("NORMAL", Float3),
                            Attribute("UV", Float2))),
                        .blendState = RTRC_STATIC_BLEND_STATE(
                        {
                            .enableBlending = true,
                            .blendingSrcColorFactor = RHI::BlendFactor::SrcAlpha,
                            .blendingDstColorFactor = RHI::BlendFactor::OneMinusSrcAlpha,
                            .blendingSrcAlphaFactor = RHI::BlendFactor::One,
                            .blendingDstAlphaFactor = RHI::BlendFactor::Zero
                        }),
                        .attachmentState = RTRC_ATTACHMENT_STATE
                        {
                            .colorAttachmentFormats = { framebuffer->GetFormat() }
                        }
                    });

                Shader::Pass passGroupData;
                passGroupData.ColorTexture = colorTexture_;
                passGroupData.SpecularTexture = specularTexture_;
                passGroupData.ObjectToClip = camera_.GetWorldToClip();
                auto passGroup = device_->CreateBindingGroupWithCachedLayout(passGroupData);

                auto &commandBuffer = RGGetCommandBuffer();
                commandBuffer.BindGraphicsPipeline(pipeline);
                commandBuffer.BindGraphicsGroups(passGroup);
                commandBuffer.SetVertexBuffer(0, vertexBuffer_, sizeof(Vertex));
                commandBuffer.Draw(vertexBuffer_->GetSize() / sizeof(Vertex), 1, 0, 0);
            });
    }

    void RenderOIT(GraphRef graph, RGTexture framebuffer)
    {
        assert(layerCount_ > 0);
        const uint32_t layerCount = (std::min)(layerCount_, 32u);

        auto counterBuffer = graph->CreateStructuredBuffer(
            framebuffer->GetWidth() * framebuffer->GetHeight(),
            sizeof(uint32_t),
            RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::ShaderRWStructuredBuffer,
            "CounterBuffer");

        auto layerBuffer = graph->CreateStructuredBuffer(
            layerCount * framebuffer->GetWidth() * framebuffer->GetHeight(),
            sizeof(uint64_t),
            RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::ShaderRWStructuredBuffer,
            "LayerBuffer");

        RGClearRWStructuredBuffer(graph, "ClearLayerBuffer", layerBuffer, 0xffffffff);

        RGClearRWStructuredBuffer(graph, "ClearCounterBuffer", counterBuffer, 0);

        {
            using Shader = RtrcShader::RenderLayers;

            Shader::Pass passData;
            passData.LayerBuffer     = layerBuffer;
            passData.CounterBuffer   = counterBuffer;
            passData.FramebufferSize = framebuffer->GetSize();
            passData.LayerCount      = layerCount;
            passData.DiscardFarest   = discardFarest_ ? 1 : 0;
            passData.ColorTexture    = colorTexture_;
            passData.SpecularTexture = specularTexture_;
            passData.ObjectToClip    = camera_.GetWorldToClip();
            passData.ObjectToView    = camera_.GetWorldToCamera();

            auto pass = graph->CreatePass("RenderLayers");
            DeclareRenderGraphResourceUses(pass, passData, RHI::PipelineStageFlag::All);
            pass->SetCallback([this, framebuffer, passData]
            {
                auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);
                
                auto pipeline = pipelineCache_.Get(
                    GraphicsPipelineDesc
                    {
                        .shader = device_->GetShader<Shader::Name>(),
                        .meshLayout = RTRC_STATIC_MESH_LAYOUT(Buffer(
                            Attribute("POSITION", Float3),
                            Attribute("NORMAL", Float3),
                            Attribute("UV", Float2))),
                    });

                auto &commandBuffer = RGGetCommandBuffer();
                commandBuffer.BindGraphicsPipeline(pipeline);
                commandBuffer.BindGraphicsGroups(passGroup);
                commandBuffer.SetViewports(framebuffer->GetViewport());
                commandBuffer.SetScissors(framebuffer->GetScissor());
                commandBuffer.SetVertexBuffer(0, vertexBuffer_, sizeof(Vertex));
                commandBuffer.Draw(vertexBuffer_->GetSize() / sizeof(Vertex), 1, 0, 0);
            });
        }

        {
            using Shader = RtrcShader::ResolveLayers;

            Shader::Pass passData;
            passData.LayerBuffer     = layerBuffer;
            passData.CounterBuffer   = counterBuffer;
            passData.FramebufferSize = framebuffer->GetSize();
            passData.LayerCount      = layerCount;

            auto pass = RGAddRenderPass<true>(
                graph, "ResolveLayers",
                RGColorAttachment
                {
                    .rtv        = framebuffer->GetRtv(),
                    .loadOp     = RHI::AttachmentLoadOp::Clear,
                    .clearValue = { 0, 1, 1, 0 },
                },
                [this, passData, framebuffer]
                {
                    auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

                    auto pipeline = pipelineCache_.Get(
                        GraphicsPipelineDesc
                        {
                            .shader = device_->GetShader<Shader::Name>(),
                            .blendState = RTRC_STATIC_BLEND_STATE(
                            {
                                .enableBlending = true,
                                .blendingSrcColorFactor = RHI::BlendFactor::SrcAlpha,
                                .blendingDstColorFactor = RHI::BlendFactor::OneMinusSrcAlpha,
                                .blendingSrcAlphaFactor = RHI::BlendFactor::One,
                                .blendingDstAlphaFactor = RHI::BlendFactor::Zero
                            }),
                            .attachmentState = RTRC_ATTACHMENT_STATE
                            {
                                .colorAttachmentFormats = { framebuffer->GetFormat() }
                            }
                        });

                    auto &commandBuffer = RGGetCommandBuffer();
                    commandBuffer.BindGraphicsPipeline(pipeline);
                    commandBuffer.BindGraphicsGroups(passGroup);
                    commandBuffer.Draw(3, 1, 0, 0);
                });

            DeclareRenderGraphResourceUses(pass, passData, RHI::PipelineStageFlag::All);
        }
    }
};

RTRC_APPLICATION_MAIN(
    MultiLayerOITDemo,
    .title             = "Rtrc Sample: Multi-layer OIT",
    .width             = 640,
    .height            = 480,
    .maximized         = true,
    .vsync             = true,
    .debug             = RTRC_DEBUG,
    .rayTracing        = false,
    .backendType       = RHI::BackendType::Default,
    .enableGPUCapturer = false)
