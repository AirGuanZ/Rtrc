#include "GridAlignment.h"
#include "Mesh.h"
#include "VDMBake.h"

#include "FADM.shader.outh"

// Feature-aware vector displacement map generation
class FADMDemo : public SimpleApplication
{
public:

    using SimpleApplication::SimpleApplication;

    enum class RenderMode
    {
        InputMesh,
        VisualizeAlignedGrid,
        UniformVDM,
        SeamCarvingByInterpolationError,
        GridAlignment,
        Count
    };

    void InitializeSimpleApplication(GraphRef graph) override
    {
        inputMesh_ = InputMesh::Load("Asset/Sample/08.FADM/Mesh1.obj");
        inputMesh_.DetectSharpFeatures(sharpEdgeAngleThreshold_);
        inputMesh_.Upload(device_);

        const Vector3f center = 0.5f * (inputMesh_.lower + inputMesh_.upper);
        const Vector3f extent = 0.5f * (inputMesh_.upper - inputMesh_.lower);
        const Vector3f lookDir = Matrix3x3f::RotateY(Deg2Rad(110)) * extent;
        camera_.SetLookAt(center + 1.5f * Vector3f(2, 1, 2) * lookDir, { 0, 1, 0 }, center);
        camera_.UpdateDerivedData();
        cameraController_.SetCamera(camera_);
        cameraController_.SetTrackballDistance(Length(camera_.GetPosition() - center));

        pipelineCache_.SetDevice(device_);

        vdmBaker_ = MakeBox<VDMBaker>(device_);

        OnGridResolutionChanged();

        GetImGuiInstance()->UseLightTheme();
    }

    void UpdateSimpleApplication(GraphRef graph) override
    {
        auto &input = GetWindowInput();
        if(input.IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        auto &imgui = *GetImGuiInstance();
        if(imgui.Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static const char *RENDER_MODE_NAMES[] =
            {
                "InputMesh",
                "VisualizeAlignedGrid",
                "UniformVDM",
                "SeamCarvingByInterpolationError",
                "GridAlignment",
            };

            if(imgui.BeginCombo("Mode", RENDER_MODE_NAMES[std::to_underlying(renderMode_)]))
            {
                for(int i = 0; i < std::to_underlying(RenderMode::Count); ++i)
                {
                    const bool selected = std::to_underlying(renderMode_) == i;
                    if(imgui.Selectable(RENDER_MODE_NAMES[i], selected))
                    {
                        renderMode_ = static_cast<RenderMode>(i);
                    }
                }
                imgui.EndCombo();
            }

            if(renderMode_ == RenderMode::SeamCarvingByInterpolationError)
            {
                if(imgui.InputVector2("Initial Sample Resolution", seamCarvingInitialResolution_))
                {
                    vdmSeamCarving_.reset();
                }
            }

            if(renderMode_ == RenderMode::VisualizeAlignedGrid)
            {
                imgui.CheckBox("Draw Input Mesh", &drawInputMeshWhenVisualizingAlignedGrid_);
            }

            if(renderMode_ == RenderMode::InputMesh || renderMode_ == RenderMode::VisualizeAlignedGrid)
            {
                imgui.CheckBox("Draw Sharp Edges", &drawSharpEdges_);
            }

            if(renderMode_ == RenderMode::VisualizeAlignedGrid || renderMode_ == RenderMode::GridAlignment)
            {
                bool reset = false;
                reset |= imgui.CheckBox("Adaptive Grid", &adaptiveGrid_);
                reset |= imgui.CheckBox("Align Corner", &alignCorner_);
                reset |= imgui.CheckBox("Align Edge", &alignEdge_);
                if(imgui.SliderAngle("Sharp Edge Angle Threshold", &sharpEdgeAngleThreshold_, 0, 180))
                {
                    inputMesh_.DetectSharpFeatures(sharpEdgeAngleThreshold_);
                    inputMesh_.Upload(device_);
                    reset = true;
                }
                if(reset)
                {
                    alignedGrid_ = {};
                    alignedGridUVTexture_ = {};
                    vdmGridAlignment_ = {};
                    alignedGridIndexBuffer_ = {};
                }
            }

            if(imgui.Input("Grid Resolution", &gridResolution_, 1, 100, nullptr, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                gridResolution_ = (std::max)(gridResolution_, 1);
                OnGridResolutionChanged();
            }
        }
        imgui.End();

        if(!imgui.IsAnyItemActive() && renderMode_ != RenderMode::InputMesh)
        {
            cameraController_.Update(GetWindowInput(), GetFrameTimer().GetDeltaSecondsF());
            camera_.SetAspectRatio(window_->GetFramebufferWOverH());
            camera_.UpdateDerivedData();
        }

        auto swapchainTexture = graph->RegisterSwapchainTexture(device_->GetSwapchain());
        auto framebuffer = graph->CreateTexture(RHI::TextureDesc
        {
            .format = RHI::Format::R8G8B8A8_UNorm,
            .width = swapchainTexture->GetWidth(),
            .height = swapchainTexture->GetHeight(),
            .usage = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
            .clearValue = RHI::ColorClearValue{ 1, 1, 1, 0 }
        }, "Framebuffer");

        if(renderMode_ == RenderMode::InputMesh)
        {
            RenderPlanarParameterizedInputMesh(framebuffer, true, false);
        }
        else if(renderMode_ == RenderMode::VisualizeAlignedGrid)
        {
            VisualizeAlignedGrid(framebuffer, true);
            if(drawInputMeshWhenVisualizingAlignedGrid_)
            {
                RenderPlanarParameterizedInputMesh(framebuffer, false, true);
            }
        }
        else if(renderMode_ == RenderMode::UniformVDM)
        {
            if(!vdmUniform_)
            {
                vdmUniform_ = vdmBaker_->BakeUniformVDM(graph, inputMesh_, Vector2u(gridResolution_ + 1));
            }
            RenderVDM(framebuffer, vdmUniform_);
        }
        else if(renderMode_ == RenderMode::SeamCarvingByInterpolationError)
        {
            if(!vdmSeamCarving_)
            {
                vdmSeamCarving_ = vdmBaker_->BakeVDMBySeamCarving(
                    graph, inputMesh_,
                    Max(seamCarvingInitialResolution_, Vector2i(1)).To<uint32_t>(),
                    Vector2u(gridResolution_ + 1));
            }
            RenderVDM(framebuffer, vdmSeamCarving_);
        }
        else if(renderMode_ == RenderMode::GridAlignment)
        {
            if(!alignedGrid_.GetWidth())
            {
                //alignedGrid_ = GridAlignment::CreateGrid(Vector2u(gridResolution_ + 1), inputMesh_, adaptiveGrid_);
                alignedGrid_ = GridAlignment::CreateGridEx(device_, pipelineCache_, Vector2u(gridResolution_ + 1), inputMesh_, adaptiveGrid_);
                if(alignCorner_)
                {
                    AlignCorners(alignedGrid_, inputMesh_);
                }
                if(alignEdge_)
                {
                    AlignSegments(alignedGrid_, inputMesh_);
                }
                alignedGridUVTexture_ = UploadGrid(device_, alignedGrid_);
                alignedGridIndexBuffer_ = GenerateIndexBufferForGrid(device_, inputMesh_, alignedGrid_);
            }
            if(!vdmGridAlignment_)
            {
                vdmGridAlignment_ = vdmBaker_->BakeAlignedVDM(graph, inputMesh_, alignedGrid_, alignedGridUVTexture_);
            }
            RenderVDM(framebuffer, vdmGridAlignment_, alignedGridIndexBuffer_);
        }

        RGBlitTexture(graph, "BlitToSwapchainTexture", framebuffer, swapchainTexture);
    }

    void RenderPlanarParameterizedInputMesh(RGTexture renderTarget, bool clearRenderTarget, bool transparent)
    {
        using Shader = RtrcShader::FADM::RenderPlanarParameterizedInputMesh;

        auto graph = renderTarget->GetGraph();
        RGAddRenderPass<true>(
            graph, Shader::Name, RGColorAttachment
            {
                .rtv = renderTarget->GetRtv(),
                .loadOp = clearRenderTarget ? RHI::AttachmentLoadOp::Clear : RHI::AttachmentLoadOp::Load,
                .clearValue = RHI::ColorClearValue{ 1, 1, 1, 0 },
                .isWriteOnly = true
            }, [renderTarget, transparent, this]
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

                {
                    Shader::Pass passData;
                    passData.ParameterSpacePositionBuffer = inputMesh_.parameterSpacePositionBuffer;
                    passData.Color = { 0, 0, 0 };
                    passData.ClipLower = -0.5f * clipRectSize;
                    passData.ClipUpper = +0.5f * clipRectSize;
                    auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

                    auto pipeline = pipelineCache_.Get(
                        {
                            .shader = device_->GetShader<Shader::Name>(),
                            .rasterizerState = RTRC_STATIC_RASTERIZER_STATE(
                            {
                                .fillMode = RHI::FillMode::Line,
                            }),
                            .blendState = RTRC_BLEND_STATE
                            {
                                .enableBlending = transparent,
                                .blendingSrcColorFactor = RHI::BlendFactor::SrcAlpha,
                                .blendingDstColorFactor = RHI::BlendFactor::OneMinusSrcAlpha,
                                .blendingSrcAlphaFactor = RHI::BlendFactor::One,
                                .blendingDstAlphaFactor = RHI::BlendFactor::Zero,
                            },
                            .attachmentState = RTRC_ATTACHMENT_STATE
                            {
                                .colorAttachmentFormats = { renderTarget->GetFormat() }
                            }
                        });

                    auto &commandBuffer = RGGetCommandBuffer();
                    commandBuffer.BindGraphicsPipeline(pipeline);
                    commandBuffer.BindGraphicsGroups(passGroup);
                    commandBuffer.SetIndexBuffer(inputMesh_.indexBuffer, RHI::IndexFormat::UInt32);
                    commandBuffer.DrawIndexed(static_cast<int>(inputMesh_.indices.size()), 1, 0, 0, 0);
                }

                if(drawSharpEdges_ && inputMesh_.sharpEdgeIndexBuffer)
                {
                    Shader::Pass passData;
                    passData.ParameterSpacePositionBuffer = inputMesh_.parameterSpacePositionBuffer;
                    passData.Color = { 1, 0, 0 };
                    passData.ClipLower = -0.5f * clipRectSize;
                    passData.ClipUpper = +0.5f * clipRectSize;
                    auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

                    auto pipeline = pipelineCache_.Get(
                        {
                            .shader = device_->GetShader<Shader::Name>(),
                            .rasterizerState = RTRC_STATIC_RASTERIZER_STATE(
                            {
                                .primitiveTopology = RHI::PrimitiveTopology::LineList
                            }),
                            .attachmentState = RTRC_ATTACHMENT_STATE
                            {
                                .colorAttachmentFormats = { renderTarget->GetFormat() }
                            }
                        });

                    auto &commandBuffer = RGGetCommandBuffer();
                    commandBuffer.BindGraphicsPipeline(pipeline);
                    commandBuffer.BindGraphicsGroups(passGroup);
                    commandBuffer.SetIndexBuffer(inputMesh_.sharpEdgeIndexBuffer, RHI::IndexFormat::UInt32);
                    commandBuffer.DrawIndexed(static_cast<int>(inputMesh_.sharpEdgeIndices.size()), 1, 0, 0, 0);
                }
            });
    }

    void OnGridResolutionChanged()
    {
        {
            auto vertexData = GenerateUniformGridVertices(gridResolution_, gridResolution_);
            gridVertexBuffer_ = device_->CreateAndUploadStructuredBuffer(
                RHI::BufferUsage::VertexBuffer, Span<Vector2i>(vertexData));
        }
        {
            auto indexData = GenerateUniformGridTriangleIndices(gridResolution_, gridResolution_);
            gridIndexBuffer_ = device_->CreateAndUploadStructuredBuffer(
                RHI::BufferUsage::IndexBuffer, Span<uint32_t>(indexData));
        }
        vdmUniform_.reset();
        vdmSeamCarving_.reset();
        vdmGridAlignment_.reset();
        alignedGrid_ = {};
        alignedGridUVTexture_ = {};
        alignedGridIndexBuffer_ = {};
    }

    void RenderVDM(RGTexture renderTarget, RC<StatefulTexture> vdm, const RC<Buffer> &overrideIndexBuffer = {})
    {
        auto graph = renderTarget->GetGraph();
        auto rgVDM = graph->RegisterTexture(vdm);

        auto depthBuffer = graph->CreateTexture(RHI::TextureDesc
        {
            .format = RHI::Format::D32,
            .width = renderTarget->GetWidth(),
            .height = renderTarget->GetHeight(),
            .usage = RHI::TextureUsage::DepthStencil,
            .clearValue = RHI::DepthStencilClearValue{ 1, 0 }
        }, "DepthBuffer");

        auto pass = RGAddRenderPass<true>(
            graph, "RenderVDM",
            RGColorAttachment
            {
                .rtv = renderTarget->GetRtv(),
                .loadOp = RHI::AttachmentLoadOp::Clear,
                .clearValue = RHI::ColorClearValue{ 1, 1, 1, 0 },
            },
            RGDepthStencilAttachment
            {
                .dsv = depthBuffer->GetDsv(),
                .loadOp = RHI::AttachmentLoadOp::Clear,
                .clearValue = RHI::DepthStencilClearValue{ 1, 0 }
            },
            [renderTarget, depthBuffer, rgVDM, overrideIndexBuffer, this]
            {
                using Shader = RtrcShader::FADM::RenderVDM;
                Shader::Pass passData;
                passData.VDM         = rgVDM;
                passData.WorldToClip = camera_.GetWorldToClip();
                passData.gridLower   = inputMesh_.gridLower;
                passData.gridUpper   = inputMesh_.gridUpper;
                passData.res         = Vector2i(gridResolution_).To<float>();
                auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

                auto pipeline = pipelineCache_.Get(
                {
                    .shader = device_->GetShader<Shader::Name>(),
                    .meshLayout = RTRC_STATIC_MESH_LAYOUT(Buffer(Attribute("POSITION", UInt2))),
                    .depthStencilState = RTRC_STATIC_DEPTH_STENCIL_STATE(
                    {
                        .enableDepthTest = true,
                        .enableDepthWrite = true,
                        .depthCompareOp = RHI::CompareOp::LessEqual
                    }),
                    .rasterizerState = RTRC_STATIC_RASTERIZER_STATE(
                    {
                        .fillMode = RHI::FillMode::Line
                    }),
                    .attachmentState = RTRC_ATTACHMENT_STATE
                    {
                        .colorAttachmentFormats = { renderTarget->GetFormat() },
                        .depthStencilFormat = depthBuffer->GetFormat()
                    }
                });

                auto &indexBuffer = overrideIndexBuffer ? overrideIndexBuffer : gridIndexBuffer_;

                auto &commandBuffer = RGGetCommandBuffer();
                commandBuffer.BindGraphicsPipeline(pipeline);
                commandBuffer.BindGraphicsGroups(passGroup);
                commandBuffer.SetVertexBuffer(0, gridVertexBuffer_, sizeof(Vector2i));
                commandBuffer.SetIndexBuffer(indexBuffer, RHI::IndexFormat::UInt32);
                commandBuffer.DrawIndexed(indexBuffer->GetSize() / sizeof(uint32_t), 1, 0, 0, 0);
            });
        pass->Use(rgVDM, RGUseInfo
        {
            .layout = RHI::TextureLayout::ShaderTexture,
            .stages = RHI::PipelineStage::VertexShader,
            .accesses = RHI::ResourceAccess::TextureRead
        });
    }

    void VisualizeAlignedGrid(RGTexture renderTarget, bool clearRenderTarget)
    {
        if(!alignedGrid_.GetWidth())
        {
            //alignedGrid_ = GridAlignment::CreateGrid(Vector2u(gridResolution_ + 1), inputMesh_, adaptiveGrid_);
            alignedGrid_ = GridAlignment::CreateGridEx(device_, pipelineCache_, Vector2u(gridResolution_ + 1), inputMesh_, adaptiveGrid_);
            if(alignCorner_)
            {
                AlignCorners(alignedGrid_, inputMesh_);
            }
            if(alignEdge_)
            {
                AlignSegments(alignedGrid_, inputMesh_);
            }
            alignedGridUVTexture_ = UploadGrid(device_, alignedGrid_);
            alignedGridIndexBuffer_ = GenerateIndexBufferForGrid(device_, inputMesh_, alignedGrid_);
        }

        auto graph = renderTarget->GetGraph();

        RGAddRenderPass<true>(
            graph, "VisualizeAlignedGrid",
            RGColorAttachment
            {
                .rtv = renderTarget->GetRtv(),
                .loadOp = clearRenderTarget ? RHI::AttachmentLoadOp::Clear : RHI::AttachmentLoadOp::Load,
                .clearValue = { 1, 1, 1, 0 }
            }, [renderTarget, this]
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

                using Shader = RtrcShader::FADM::VisualizeAlignedGrid;

                Shader::Pass passData;
                passData.GridUVTexture = alignedGridUVTexture_;
                passData.clipLower = -0.5f * clipRectSize;
                passData.clipUpper = +0.5f * clipRectSize;
                passData.color = { 0, 1, 0 };
                auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

                auto pipeline = pipelineCache_.Get(
                    {
                        .shader = device_->GetShader<Shader::Name>(),
                        .meshLayout = RTRC_STATIC_MESH_LAYOUT(Buffer(Attribute("POSITION", UInt2))),
                        .rasterizerState = RTRC_STATIC_RASTERIZER_STATE(
                        {
                            .primitiveTopology = RHI::PrimitiveTopology::TriangleList,
                            .fillMode = RHI::FillMode::Line
                        }),
                        .attachmentState = RTRC_ATTACHMENT_STATE
                        {
                            .colorAttachmentFormats = { renderTarget->GetFormat() }
                        }
                    });

                auto &commandBuffer = RGGetCommandBuffer();
                commandBuffer.BindGraphicsPipeline(pipeline);
                commandBuffer.BindGraphicsGroups(passGroup);
                commandBuffer.SetVertexBuffer(0, gridVertexBuffer_, sizeof(Vector2i));
                commandBuffer.SetIndexBuffer(alignedGridIndexBuffer_, RHI::IndexFormat::UInt32);
                commandBuffer.DrawIndexed(
                    static_cast<int>(alignedGridIndexBuffer_->GetSize() / sizeof(uint32_t)), 1, 0, 0, 0);
            });
    }

    InputMesh             inputMesh_;
    GraphicsPipelineCache pipelineCache_;

    Camera camera_;
    EditorCameraController cameraController_;

    RenderMode renderMode_ = RenderMode::InputMesh;

    Box<VDMBaker> vdmBaker_;

    Vector2i seamCarvingInitialResolution_ = { 256, 256 };

    int gridResolution_ = 32;
    RC<Buffer> gridVertexBuffer_;
    RC<Buffer> gridIndexBuffer_;
    RC<StatefulTexture> vdmUniform_;
    RC<StatefulTexture> vdmSeamCarving_;
    RC<StatefulTexture> vdmGridAlignment_;

    float sharpEdgeAngleThreshold_ = Deg2Rad(40);
    Image<GridAlignment::GridPoint> alignedGrid_;
    RC<Texture> alignedGridUVTexture_;
    RC<Buffer> alignedGridIndexBuffer_;

    bool drawSharpEdges_ = true;

    bool adaptiveGrid_ = true;
    bool alignCorner_ = true;
    bool alignEdge_ = true;
    bool drawInputMeshWhenVisualizingAlignedGrid_ = true;
};

RTRC_APPLICATION_MAIN(
    FADMDemo,
    .title             = "Rtrc Sample: FADM",
    .width             = 640,
    .height            = 480,
    .maximized         = true,
    .vsync             = true,
    .debug             = RTRC_DEBUG,
    .rayTracing        = false,
    .backendType       = RHI::BackendType::Default,
    .enableGPUCapturer = false)
