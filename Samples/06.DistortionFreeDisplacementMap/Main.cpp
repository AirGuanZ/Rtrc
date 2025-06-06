#include <Rtrc/Rtrc.h>

#include "Grid.shader.outh"

using namespace Rtrc;

class DFDMDemo : public SimpleApplication
{
public:

    using SimpleApplication::SimpleApplication;

    static constexpr int N = 2048;
    static constexpr float WorldSize = 10.0f;
    static constexpr float HeightScale = 2.0f;

    void InitializeSimpleApplication(GraphRef graph) override
    {
        InitializeGeometryBuffers();
        InitializeGeometryMaps();
        InitializeRenderPipeline();

        camera_.SetLookAt({ -2, 5, -2 }, { 0, 1, 0 }, { 5, 1, 5 });
        cameraController_.SetCamera(camera_);
        cameraController_.SetTrackballDistance(Length(camera_.GetPosition() - Vector3f(5, 1, 5)));

        gizmoRenderer_ = MakeBox<GizmoRenderer>(device_);
    }

    void UpdateSimpleApplication(GraphRef renderGraph) override
    {
        if(input_->IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        bool reinitGeometryImage = false;

        auto &imgui = *GetImGuiInstance();
        if(imgui.Begin("DFDM Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            imgui.CheckBox("Enable Correction", &enableCorrection_);
            imgui.CheckBox("Enable Checkboard", &enableCheckboard_);
            if(imgui.Input("Area Preservation", &areaPreservation_, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
                reinitGeometryImage = true;
        }
        imgui.End();

        if(reinitGeometryImage)
        {
            InitializeGeometryMaps();
        }

        cameraController_.Update(GetWindowInput(), GetFrameTimer().GetDeltaSecondsF());
        camera_.SetAspectRatio(window_->GetFramebufferWOverH());
        camera_.UpdateDerivedData();

        auto swapchainTexture = renderGraph->RegisterSwapchainTexture(device_->GetSwapchain());
        auto framebuffer = renderGraph->CreateTexture(RHI::TextureDesc
        {
            .format     = RHI::Format::R8G8B8A8_UNorm,
            .width      = swapchainTexture->GetWidth(),
            .height     = swapchainTexture->GetHeight(),
            .usage      = RHI::TextureUsage::RenderTarget |
                          RHI::TextureUsage::ShaderResource,
            .clearValue = RHI::ColorClearValue{ 0, 0, 0, 0 }
        }, "Framebuffer");

        auto depthBuffer = renderGraph->CreateTexture(RHI::TextureDesc
        {
            .format     = RHI::Format::D24S8,
            .width      = framebuffer->GetWidth(),
            .height     = framebuffer->GetHeight(),
            .usage      = RHI::TextureUsage::DepthStencil,
            .clearValue = RHI::DepthStencilClearValue{ 1, 0 }
        });

        {
            auto renderPass = renderGraph->CreatePass("RenderMesh");
            renderPass->Use(framebuffer, RG::ColorAttachment);
            renderPass->Use(depthBuffer, RG::DepthStencilAttachment);
            renderPass->SetCallback([framebuffer, depthBuffer, this]
            {
                auto &cmds = RGGetCommandBuffer();
                cmds.BeginRenderPass(
                    RenderTargetBinding::CreateClear(framebuffer),
                    DepthStencilBinding::CreateClear(depthBuffer));
                RTRC_SCOPE_EXIT{ cmds.EndRenderPass(); };

                StaticShaderInfo<"DFDM/Render">::Pass passData;
                passData.HeightMap        = height_;
                passData.NormalMap        = normal_;
                passData.CorrectionMap    = correction_;
                passData.ColorMap         = color_;
                passData.WorldToClip      = camera_.GetWorldToClip();
                passData.WorldSize        = WorldSize;
                passData.N                = N;
                passData.EnableCorrection = enableCorrection_;
                passData.Checkboard       = enableCheckboard_;
                auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

                cmds.BindGraphicsPipeline(pipeline_);
                cmds.BindGraphicsGroup(0, passGroup);
                cmds.SetViewports(framebuffer->GetViewport());
                cmds.SetScissors(framebuffer->GetScissor());
                cmds.SetVertexBuffer(0, vertexBuffer_, sizeof(Vector2f));
                cmds.SetIndexBuffer(indexBuffer_, RHI::IndexFormat::UInt32);
                cmds.DrawIndexed(static_cast<int>(indexBuffer_->GetSize() / sizeof(uint32_t)), 1, 0, 0, 0);
            });
        }

        {
            GizmoBuilder gizmo;
            gizmo.PushColor({ 0, 1, 1 });
            gizmo.DrawWireCuboid(
                Vector3f(0),
                Vector3f(WorldSize, 0, 0),
                Vector3f(0, HeightScale, 0),
                Vector3f(0, 0, WorldSize));
            gizmoRenderer_->AddRenderPass(
                gizmo, renderGraph, framebuffer, depthBuffer, camera_.GetWorldToClip(), false, 2.2f);
        }

        RGBlitTexture(renderGraph, "BlitToSwapchainTexture", framebuffer, swapchainTexture);
    }

    void InitializeGeometryBuffers()
    {
        std::vector<Vector2f> vertexData((N + 1) * (N + 1));
        for(int y = 0; y <= N; ++y)
        {
            for(int x = 0; x <= N; ++x)
            {
                const int i = y * (N + 1) + x;
                vertexData[i] = Vector2i(x, y).To<float>();
            }
        }
        vertexBuffer_ = device_->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size = sizeof(Vector2f) * vertexData.size(),
            .usage = RHI::BufferUsage::VertexBuffer
        }, vertexData.data());
        vertexBuffer_->SetName("VertexBuffer");

        std::vector<uint32_t> indexData;
        indexData.reserve(N * N * 6);
        for(int y = 0; y < N; ++y)
        {
            for(int x = 0; x < N; ++x)
            {
                indexData.push_back((y + 0) * (N + 1) + (x + 0));
                indexData.push_back((y + 1) * (N + 1) + (x + 0));
                indexData.push_back((y + 1) * (N + 1) + (x + 1));
                indexData.push_back((y + 0) * (N + 1) + (x + 0));
                indexData.push_back((y + 1) * (N + 1) + (x + 1));
                indexData.push_back((y + 0) * (N + 1) + (x + 1));
            }
        }
        indexBuffer_ = device_->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size = sizeof(uint32_t)*  indexData.size(),
            .usage = RHI::BufferUsage::IndexBuffer
        }, indexData.data());
        indexBuffer_->SetName("IndexBuffer");
    }

    void InitializeGeometryMaps()
    {
        // Height map

        constexpr char heightMapFilename[] = "Asset/Sample/06.DistortionFreeDisplacementMap/Height.png";
        auto heightMap = ImageDynamic::Load(heightMapFilename).To<float>();
        for(float& height : heightMap)
        {
            height *= HeightScale;
        }
        height_ = device_->CreateAndUploadTexture2D(RHI::TextureDesc
        {
            .format = RHI::Format::R32_Float,
            .width  = heightMap.GetWidth(),
            .height = heightMap.GetHeight(),
            .usage  = RHI::TextureUsage::ShaderResource
        }, ImageDynamic(heightMap), RHI::TextureLayout::ShaderTexture);
        height_->SetName("HeightMap");

        // Correction map

        Image<Vector3f> displacementMap(heightMap.GetWidth(), heightMap.GetHeight());
        const float displacementScale = static_cast<float>(heightMap.GetWidth()) / WorldSize;
        for(unsigned int y = 0; y < displacementMap.GetHeight(); ++y)
        {
            for(unsigned int x = 0; x < displacementMap.GetWidth(); ++x)
            {
                displacementMap(x, y) = Vector3f(0, displacementScale * heightMap(x, y), 0);
            }
        }

        DFDM dfdm;
        dfdm.SetDevice(device_);
        dfdm.SetIterationCount(300);
        dfdm.SetAreaPreservation(areaPreservation_);
        auto correctionMap = dfdm.GenerateCorrectionMap(displacementMap);
        const float offsetScale = 1.0f / static_cast<float>(correctionMap.GetWidth());
        for(Vector2f &offset : correctionMap)
        {
            offset = offsetScale * offset;
        }
        correction_ = device_->CreateAndUploadTexture2D(RHI::TextureDesc
        {
            .format = RHI::Format::R32G32_Float,
            .width  = correctionMap.GetWidth(),
            .height = correctionMap.GetHeight(),
            .usage  = RHI::TextureUsage::ShaderResource
        }, ImageDynamic(correctionMap), RHI::TextureLayout::ShaderTexture);
        correction_->SetName("CorrectionMap");

        // Normal map

        auto GetHeight = [&](int x, int y)
        {
            x = std::clamp<int>(x, 0, heightMap.GetSWidth() - 1);
            y = std::clamp<int>(y, 0, heightMap.GetSHeight() - 1);
            return heightMap(x, y);
        };

        Image<Vector3f> normalMap(heightMap.GetWidth(), heightMap.GetHeight());
        for(int y = 0; y < normalMap.GetSHeight(); ++y)
        {
            for(int x = 0; x < normalMap.GetSWidth(); ++x)
            {
                const float dx = 2.0f * (GetHeight(x + 1, y + 0) - GetHeight(x - 1, y + 0)) +
                                 1.0f * (GetHeight(x + 1, y + 1) - GetHeight(x - 1, y + 1)) +
                                 1.0f * (GetHeight(x + 1, y - 1) - GetHeight(x - 1, y - 1));
                const float dy = 2.0f * (GetHeight(x + 0, y + 1) - GetHeight(x + 0, y - 1)) +
                                 1.0f * (GetHeight(x + 1, y + 1) - GetHeight(x + 1, y - 1)) +
                                 1.0f * (GetHeight(x - 1, y + 1) - GetHeight(x - 1, y - 1));
                const float step = WorldSize / static_cast<float>(heightMap.GetWidth());
                const Vector3f n = { -dy, 8 * step, -dx };
                normalMap(x, y) = Normalize(n);
            }
        }

        normal_ = device_->CreateAndUploadTexture2D(RHI::TextureDesc
        {
            .format = RHI::Format::R32G32B32A32_Float,
            .width  = normalMap.GetWidth(),
            .height = normalMap.GetHeight(),
            .usage  = RHI::TextureUsage::ShaderResource
        }, ImageDynamic(normalMap), RHI::TextureLayout::ShaderTexture);
        normal_->SetName("NormalMap");

        // Color map

        color_ = device_->LoadTexture2D(
            "Asset/Sample/06.DistortionFreeDisplacementMap/Color.png",
            RHI::Format::R8G8B8A8_UNorm, RHI::TextureUsage::ShaderResource,
            true, RHI::TextureLayout::ShaderTexture);
    }

    void InitializeRenderPipeline()
    {
        auto layout = RTRC_MESH_LAYOUT(Buffer(Attribute("Position", 0, Float2)));
        const GraphicsPipelineDesc pipelineDesc =
        {
            .shader            = device_->GetShader<RtrcShader::DFDM::Render::Name>(),
            .meshLayout        = layout,
            .depthStencilState = RTRC_STATIC_DEPTH_STENCIL_STATE(
            {
                .enableDepthTest = true,
                .enableDepthWrite = true,
                .depthCompareOp = RHI::CompareOp::Less,
            }),
            .rasterizerState = RTRC_STATIC_RASTERIZER_STATE(
            {
                .cullMode = RHI::CullMode::CullBack,
                .frontFaceMode = RHI::FrontFaceMode::CounterClockwise,
            }),
            .attachmentState = RTRC_STATIC_ATTACHMENT_STATE(
            {
                .colorAttachmentFormats = { RHI::Format::R8G8B8A8_UNorm },
                .depthStencilFormat = RHI::Format::D24S8
            })
        };
        pipeline_ = device_->CreateGraphicsPipeline(pipelineDesc);
    }
    
    RC<Buffer> vertexBuffer_;
    RC<Buffer> indexBuffer_;

    RC<Texture> color_;
    RC<Texture> correction_;
    RC<Texture> height_;
    RC<Texture> normal_;

    RC<GraphicsPipeline> pipeline_;

    Camera camera_;
    EditorCameraController cameraController_;

    float areaPreservation_ = 3.0f;
    bool enableCorrection_ = true;
    bool enableCheckboard_ = true;

    Box<GizmoRenderer> gizmoRenderer_;
};

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        DFDMDemo().Run(DFDMDemo::Config
        {
            .title             = "Rtrc Sample: Distortion-Free Displacement Map",
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
