#include <Rtrc/Rtrc.h>

#include "Bake.shader.outh"
#include "Render.shader.outh"

using namespace Rtrc;

class HorizonShadowMapDemo : public SimpleApplication
{
    void InitializeSimpleApplication(GraphRef graph) override
    {
        ReloadHeightMap(graph);

        camera_.SetLookAt(0.7f * Vector3f(-terrainSize_, terrainSize_, -terrainSize_), { 0, 1, 0 }, { 0, 0, 0 });
        cameraController_.SetCamera(camera_);
        cameraController_.SetTrackballDistance(Length(camera_.GetPosition()));
    }

    void UpdateSimpleApplication(GraphRef graph) override
    {
        if(GetWindowInput().IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        auto &imgui = *GetImGuiInstance();
        if(imgui.Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            imgui.SliderAngle("SunAngle", &sunThetaRad_, 0.0f, 90.0f);
            imgui.Input("Softness", &softnessDeg_);

            if(imgui.Input("Height Scale", &heightScale_, nullptr, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                heightScale_ = std::clamp(heightScale_, 0.0f, 1024.0f);
                ReloadHeightMap(graph);
            }
        }
        imgui.End();

        camera_.SetAspectRatio(GetWindow().GetFramebufferWOverH());
        if(!imgui.IsAnyItemActive())
        {
            cameraController_.Update(GetWindowInput(), GetFrameTimer().GetDeltaSecondsF());
        }
        camera_.UpdateDerivedData();

        auto framebuffer = graph->RegisterSwapchainTexture(GetSwapchain());
        Render(graph, framebuffer);
    }

    void ReloadHeightMap(GraphRef graph)
    {
        auto heightMapData = Image<float>::Load("Asset/Sample/13.HorizonShadowMap/Height.png");
        float minHeight = FLT_MAX;
        for(float h : heightMapData)
        {
            minHeight = (std::min)(minHeight, h);
        }
        for(float &h : heightMapData)
        {
            h = (h - minHeight) * heightScale_;
        }

        auto LoadHeight =
            [&, sx = heightMapData.GetSWidthMinusOne(), sy = heightMapData.GetSHeightMinusOne()]
            (int x, int y)
        {
            x = std::clamp(x, 0, sx);
            y = std::clamp(y, 0, sy);
            return heightMapData(x, y);
        };

        const float horizontalStepSize = terrainSize_ / (std::max)(heightMapData.GetWidth(), heightMapData.GetHeight());
        auto GetPosition = [&](int x, int y)
        {
            return Vector3f(x * horizontalStepSize, LoadHeight(x, y), y * horizontalStepSize);
        };

        Image<Vector3f> normalMapData(heightMapData.GetWidth(), heightMapData.GetHeight());
        ParallelFor(0, normalMapData.GetSHeight(), [&](int y)
        {
            for(int x = 0; x < normalMapData.GetSWidth(); ++x)
            {
                const Vector3f dpdu =
                    1.0f / 4.0f * (GetPosition(x + 1, y - 1) - GetPosition(x - 1, y - 1)) +
                    1.0f / 2.0f * (GetPosition(x + 1, y + 0) - GetPosition(x - 1, y + 0)) +
                    1.0f / 4.0f * (GetPosition(x + 1, y + 1) - GetPosition(x - 1, y + 1));
                const Vector3f dpdv =
                    1.0f / 4.0f * (GetPosition(x - 1, y + 1) - GetPosition(x - 1, y - 1)) +
                    1.0f / 2.0f * (GetPosition(x + 0, y + 1) - GetPosition(x + 0, y - 1)) +
                    1.0f / 4.0f * (GetPosition(x + 1, y + 1) - GetPosition(x + 1, y - 1));
                const Vector3f normal = Normalize(Cross(dpdv, dpdu));
                normalMapData(x, y) = normal;
            }
        });

        heightMap_ = GetDevice()->CreateAndUploadTexture(
            RHI::TextureDesc
            {
                .format = RHI::Format::R32_Float,
                .width  = heightMapData.GetWidth(),
                .height = heightMapData.GetHeight(),
                .usage  = RHI::TextureUsage::ShaderResource
            }, heightMapData.GetData(), RHI::TextureLayout::ShaderTexture);

        normalMap_ = GetDevice()->CreateAndUploadTexture(
            RHI::TextureDesc
            {
                .format = RHI::Format::R32G32B32_Float,
                .width  = normalMapData.GetWidth(),
                .height = normalMapData.GetHeight(),
                .usage  = RHI::TextureUsage::ShaderResource
            }, normalMapData.GetData(), RHI::TextureLayout::ShaderTexture);

        horizonMap_ = GetDevice()->CreateStatefulTexture(
            RHI::TextureDesc
            {
                .format = RHI::Format::R32_Float,
                .width  = heightMap_->GetWidth(),
                .height = heightMap_->GetHeight(),
                .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderedAccess
            }, "VisibleAngleMap");

        auto horizonMap = graph->RegisterTexture(horizonMap_);

        RtrcShader::Bake::Pass passData;
        passData.HeightMap          = heightMap_;
        passData.NormalMap          = normalMap_;
        passData.MinVisibleAngleMap = horizonMap;
        passData.resolution         = horizonMap->GetSize();
        passData.horizontalScale    = horizontalStepSize;
        passData.sunPhi             = 0;

        RGDispatchWithThreadCount(
            graph,
            "GenerateHorizonMap",
            GetDevice()->GetShader<RtrcShader::Bake::Name>(),
            horizonMap->GetSize(),
            passData);
    }

    void Render(GraphRef graph, RGTexture renderTarget)
    {
        auto depthBuffer = graph->CreateTexture(
            RHI::TextureDesc
            {
                .format     = RHI::Format::D32,
                .width      = renderTarget->GetWidth(),
                .height     = renderTarget->GetHeight(),
                .usage      = RHI::TextureUsage::DepthStencil,
                .clearValue = RHI::DepthStencilClearValue{ 1, 0 }
            }, "DepthBuffer");

        if(!renderPipeline_)
        {
            renderPipeline_ = GetDevice()->CreateGraphicsPipeline(
                GraphicsPipelineDesc
                {
                    .shader = GetDevice()->GetShader<RtrcShader::Render::Name>(),
                    .depthStencilState = RTRC_DEPTH_STENCIL_STATE
                    {
                        .enableDepthTest  = true,
                        .enableDepthWrite = true,
                        .depthCompareOp   = RHI::CompareOp::LessEqual
                    },
                    .attachmentState = RTRC_ATTACHMENT_STATE
                    {
                        .colorAttachmentFormats = { renderTarget->GetFormat() },
                        .depthStencilFormat     = depthBuffer->GetFormat()
                    }
                });
        }

        if(!indexBuffer_)
        {
            const auto indexData = GenerateUniformGridTriangleIndices<uint32_t>(N, N);
            indexBuffer_ = GetDevice()->CreateAndUploadBuffer(
                RHI::BufferDesc
                {
                    .size = indexData.size() * sizeof(uint32_t),
                    .usage = RHI::BufferUsage::IndexBuffer
                }, indexData.data());
            indexCount_ = indexData.size();
        }

        RGAddRenderPass<true>(
            graph, "Render", RGColorAttachment
            {
                .rtv        = renderTarget->GetRtv(),
                .loadOp     = RHI::AttachmentLoadOp::Clear,
                .clearValue = RHI::ColorClearValue{ 0, 0, 0, 0 }
            },
            RGDepthStencilAttachment
            {
                .dsv        = depthBuffer->GetDsv(),
                .loadOp     = RHI::AttachmentLoadOp::Clear,
                .clearValue = RHI::DepthStencilClearValue{ 1, 0 }
            },
            [this]
            {
                RtrcShader::Render::Pass passData;
                passData.HeightMap       = heightMap_;
                passData.NormalMap       = normalMap_;
                passData.HorizonMap      = horizonMap_;
                passData.worldToClip     = camera_.GetWorldToClip();
                passData.horizontalLower = -Vector2f(0.5f * terrainSize_);
                passData.horizontalUpper = Vector2f(0.5f * terrainSize_);
                passData.N               = N;
                passData.sunTheta        = sunThetaRad_;
                passData.softness        = Deg2Rad(std::clamp(softnessDeg_, 1e-4f, 90.0f));
                auto passGroup = GetDevice()->CreateBindingGroupWithCachedLayout(passData);

                auto &commandBuffer = RGGetCommandBuffer();
                commandBuffer.BindGraphicsPipeline(renderPipeline_);
                commandBuffer.BindGraphicsGroups(passGroup);
                commandBuffer.SetIndexBuffer(indexBuffer_, RHI::IndexFormat::UInt32);
                commandBuffer.DrawIndexed(indexCount_, 1, 0, 0, 0);
            });
    }

    static constexpr uint32_t N = 1024;

    float terrainSize_ = 128;
    float heightScale_ = 32;
    RC<Texture> heightMap_;
    RC<Texture> normalMap_;
    RC<StatefulTexture> horizonMap_;

    float sunThetaRad_ = Deg2Rad(30);
    float softnessDeg_ = 16;

    RC<Buffer> indexBuffer_;
    uint32_t indexCount_ = 0;

    Camera camera_;
    EditorCameraController cameraController_;

    RC<GraphicsPipeline> renderPipeline_;
};

RTRC_APPLICATION_MAIN(
    HorizonShadowMapDemo,
    .title             = "Rtrc Sample: Horizon Shadow Map",
    .width             = 640,
    .height            = 480,
    .maximized         = true,
    .vsync             = true,
    .debug             = RTRC_DEBUG,
    .rayTracing        = false,
    .backendType       = RHI::BackendType::Default,
    .enableGPUCapturer = false)
