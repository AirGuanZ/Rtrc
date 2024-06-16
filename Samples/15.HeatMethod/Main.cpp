#include <Rtrc/Geometry/DiscreteOperators.h>
#include <Rtrc/Rtrc.h>

#include "Visualize.shader.outh"

#include <iostream>

using namespace Rtrc;

using SparseMatrixXd = Eigen::SparseMatrix<double>;

class HeatMethodDemo : public SimpleApplication
{
    static void PrintSparseMatrix(const Eigen::MatrixXd &s)
    {
        const Eigen::MatrixXd m = s;
        std::cout << m << "\n";
    }

    void InitializeSimpleApplication(GraphRef graph) override
    {
        rawMesh_ = RawMesh::Load("./Asset/Sample/15.HeatMethod/Bunny_good.obj");
        rawMesh_.NormalizePositionTo({ -1, -1, -1 }, { 1, 1, 1 });
        rawMesh_.RecalculateNormal(0);

        camera_.SetLookAt({ -3, 0, 0 }, { 0, 1, 0 }, { 0, 0, 0 });
        cameraController_.SetCamera(camera_);
        cameraController_.SetTrackballDistance(Length(camera_.GetPosition()));

        PreprocessMesh();

        int sourceVertex = 0;
        for(int v = 0; v < halfEdgeMesh_.V(); ++v)
        {
            if(!halfEdgeMesh_.IsVertOnBoundary(v)) // Set the first non-boundary vertex as heat source
            {
                sourceVertex = v;
                break;
            }
        }

        ComputeGeodesicDistance(sourceVertex);
    }

    void UpdateSimpleApplication(GraphRef graph) override
    {
        if(GetWindowInput().IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        if(imgui_->Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            int mode = visualizationMode_;
            if(imgui_->Combo("Visualization Mode", &mode, { "Heat", "Gradient", "Divergence", "Distance" }))
            {
                visualizationMode_ = static_cast<VisualizeMode>(mode);
            }
        }
        imgui_->End();

        camera_.SetAspectRatio(GetWindow().GetFramebufferWOverH());
        if(!imgui_->IsAnyItemActive())
        {
            cameraController_.Update(GetWindowInput(), GetFrameTimer().GetDeltaSecondsF());
            camera_.UpdateDerivedData();
        }

        if(!imgui_->IsAnyItemActive() &&
           !GetWindowInput().IsKeyPressed(KeyCode::LeftAlt) &&
           GetWindowInput().IsKeyDown(KeyCode::MouseLeft))
        {
            const float tx = GetWindowInput().GetCursorAbsolutePositionX() / GetWindow().GetFramebufferSize().x;
            const float ty = GetWindowInput().GetCursorAbsolutePositionY() / GetWindow().GetFramebufferSize().y;
            auto worldRays = camera_.GetWorldRays();
            const Vector3f direction =
                Normalize(Lerp(
                    Lerp(worldRays[0], worldRays[1], tx),
                    Lerp(worldRays[2], worldRays[3], tx),
                    ty));
            const Vector3f origin = camera_.GetPosition();

            bvh_.FindClosestIntersection(
                origin, direction, FLT_MAX, [&](float t, uint32_t triangle, const Vector2f &uv)
                {
                    float bestDistance = FLT_MAX; int bestV = -1;
                    for(int i = 0; i < 3; ++i)
                    {
                        const int v = halfEdgeMesh_.Vert(3 * triangle + i);
                        if(halfEdgeMesh_.IsVertOnBoundary(v))
                        {
                            continue;
                        }
                        const Vector3f op = rawMesh_.GetPositionData()[v] - origin;
                        const Vector3f diff = op - Dot(op, direction) * direction;
                        const float distance = Length(diff);
                        if(distance < bestDistance)
                        {
                            bestDistance = distance;
                            bestV = v;
                        }
                    }

                    if(bestV >= 0)
                    {
                        ComputeGeodesicDistance(bestV);
                    }
                });
        }

        auto framebuffer = graph->RegisterSwapchainTexture(GetSwapchain());
        VisualizeHeat(graph, framebuffer);
    }

    void PreprocessMesh()
    {
        const std::vector<Vector3d> positions =
            rawMesh_.GetPositionData() |
            std::views::transform([](const Vector3f &p) { return p.To<double>(); }) |
            std::ranges::to<std::vector<Vector3d>>();

        bvh_ = TriangleBVH::Build(rawMesh_.GetPositionData(), rawMesh_.GetPositionIndices());

        halfEdgeMesh_ = FlatHalfedgeMesh::Build(rawMesh_.GetPositionIndices());

        auto &m = halfEdgeMesh_;

        double dt;
        {
            double sumEdgeLength = 0;
            for(int e = 0; e < m.E(); ++e)
            {
                const int h = m.EdgeToHalfedge(e);
                const int v0 = m.Head(h);
                const int v1 = m.Tail(h);
                const Vector3d p0 = positions[v0];
                const Vector3d p1 = positions[v1];
                sumEdgeLength += Length(p0 - p1);
            }
            const double meanEdgeLength = sumEdgeLength / m.E();
            dt = meanEdgeLength * meanEdgeLength;
        }
        LogInfo("Time step = {}", dt);

        const SparseMatrixXd A = (1.0 / 3.0) * BuildVertexAreaDiagonalMatrix<double>(m, positions);
        const SparseMatrixXd Ld = BuildCotanLaplacianMatrix<double>(m, positions, CotanLaplacianBoundaryType::Dirichlet);
        const SparseMatrixXd Ln = BuildCotanLaplacianMatrix<double>(m, positions, CotanLaplacianBoundaryType::Neumann);

        dirichletHeatSolver_.compute(A - dt * Ld);
        neumannHeatSolver_.compute(A - dt * Ln);

        gradientOperatorX_ = BuildFaceGradientMatrix_X<double>(m, positions);
        gradientOperatorY_ = BuildFaceGradientMatrix_Y<double>(m, positions);
        gradientOperatorZ_ = BuildFaceGradientMatrix_Z<double>(m, positions);

        divergenceOperator_ = BuildVertexDivergenceMatrix<double>(m, positions);

        poissonSolver_.compute(Ln);
    }

    void ComputeGeodesicDistance(int sourceVertex)
    {
        auto &m = halfEdgeMesh_;

        // Heat diffusion

        Eigen::VectorXd delta = Eigen::VectorXd::Zero(m.V());
        delta(sourceVertex) = 1.0;

        const Eigen::VectorXd ud = dirichletHeatSolver_.solve(delta);
        const Eigen::VectorXd un = neumannHeatSolver_.solve(delta);
        const Eigen::VectorXd u = 0.5 * (ud + un);

        // Gradient

        const Eigen::VectorXd gradientX = -gradientOperatorX_ * u;
        const Eigen::VectorXd gradientY = -gradientOperatorY_ * u;
        const Eigen::VectorXd gradientZ = -gradientOperatorZ_ * u;
        
        std::vector<Vector3d> gradients(m.F());
        for(int f = 0; f < m.F(); ++f)
        {
            gradients[f] = NormalizeIfNotZero(Vector3d
            {
                gradientX[f],
                gradientY[f],
                gradientZ[f],
            });
        }

        // Divergence

        Eigen::VectorXd flattenGradients(m.F() * 3);
        for(int f = 0; f < m.F(); ++f)
        {
            flattenGradients[3 * f + 0] = gradients[f].x;
            flattenGradients[3 * f + 1] = gradients[f].y;
            flattenGradients[3 * f + 2] = gradients[f].z;
        }
        const Eigen::VectorXd divergence = divergenceOperator_ * flattenGradients;

        // Poisson

        const Eigen::VectorXd phi = poissonSolver_.solve(divergence);

        // Geodesic distances

        const double minPhi = *std::ranges::fold_left_first(phi, [](double a, double b) { return (std::min)(a, b); });
        std::vector<double> geodesicDistances = { phi.begin(), phi.end() };
        for(double &d : geodesicDistances)
        {
            d -= minPhi;
        }

        // Visualization

        auto minOperator = [](double a, double b) { return (std::min)(a, b); };
        auto maxOperator = [](double a, double b) { return (std::max)(a, b); };

        const double minHeat = *std::ranges::fold_left_first(u, minOperator);
        const double maxHeat = *std::ranges::fold_left_first(u, maxOperator);
        LogInfo("Min heat = {}, max heat = {}", minHeat, maxHeat);

        const double minDivergence = *std::ranges::fold_left_first(divergence, minOperator);
        const double maxDivergence = *std::ranges::fold_left_first(divergence, maxOperator);
        LogInfo("Min divergence = {}, max divergence = {}", minDivergence, maxDivergence);
        const double maxAbsDivergence = (std::max)(std::abs(minDivergence), std::abs(maxDivergence));
        
        std::vector<Vector3f> vertexGradients(m.V());
        for(int v = 0; v < m.V(); ++v)
        {
            Vector3d sumWeightedGradients; double sumWeights = 0;
            m.ForEachOutgoingHalfedge(v, [&](int h)
            {
                const int f = h / 3;
                const int h0 = h;
                const int h1 = m.Succ(h0);
                const int h2 = m.Succ(h1);
                const Vector3f p0 = rawMesh_.GetPositionData()[m.Vert(h0)];
                const Vector3f p1 = rawMesh_.GetPositionData()[m.Vert(h1)];
                const Vector3f p2 = rawMesh_.GetPositionData()[m.Vert(h2)];
                const double cosAngle = Cos(p1 - p0, p2 - p0);
                const double angle = std::acos(Clamp(cosAngle, -1.0, 1.0));
                sumWeightedGradients += angle * gradients[f];
                sumWeights += angle;
            });
            vertexGradients[v] = NormalizeIfNotZero(sumWeightedGradients).To<float>();
        }
        
        // Render data

        std::vector<Vertex> vertexData;
        vertexData.resize(m.V());
        for(int v = 0; v < m.V(); ++v)
        {
            Vertex &vertex = vertexData[v];
            vertex.position = rawMesh_.GetPositionData()[v];
            vertex.normalizedHeat = static_cast<float>(Saturate(InverseLerp(minHeat, maxHeat, u[v])));
            vertex.gradient = vertexGradients[v];
            vertex.divergence = static_cast<float>(divergence[v] / maxAbsDivergence);
            vertex.distance = static_cast<float>(geodesicDistances[v]);
        }

        vertexBuffer_ = device_->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size = sizeof(Vertex) * vertexData.size(),
            .usage = RHI::BufferUsage::VertexBuffer
        }, vertexData.data());
        indexBuffer_ = device_->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size = sizeof(uint32_t) * rawMesh_.GetPositionIndices().size(),
            .usage = RHI::BufferUsage::IndexBuffer
        }, rawMesh_.GetPositionIndices().GetData());
    }

    void VisualizeHeat(GraphRef graph, RGTexture renderTarget)
    {
        auto depthBuffer = graph->CreateTexture(RHI::TextureDesc
        {
            .format = RHI::Format::D32,
            .width = renderTarget->GetWidth(),
            .height = renderTarget->GetHeight(),
            .usage = RHI::TextureUsage::DepthStencil,
            .clearValue = RHI::DepthStencilClearValue{ 1, 0 }
        });

        if(!visualizeHeatPipeline_)
        {
            visualizeHeatPipeline_ = device_->CreateGraphicsPipeline(GraphicsPipelineDesc
            {
                .shader = device_->GetShader<RtrcShader::VisualizeHeat::Name>(),
                .meshLayout = RTRC_MESH_LAYOUT(Buffer(
                    Attribute("POSITION", Float3),
                    Attribute("HEAT", Float),
                    Attribute("GRADIENT", Float3),
                    Attribute("DIVERGENCE", Float),
                    Attribute("DISTANCE", Float))),
                .depthStencilState = RTRC_DEPTH_STENCIL_STATE
                {
                    .enableDepthTest = true,
                    .enableDepthWrite = true,
                    .depthCompareOp = RHI::CompareOp::LessEqual
                },
                .attachmentState = RTRC_ATTACHMENT_STATE
                {
                    .colorAttachmentFormats = { renderTarget->GetFormat() },
                    .depthStencilFormat = depthBuffer->GetFormat()
                }
            });
        }

        RGAddRenderPass<true>(
            graph, "Visualize",
            RGColorAttachment
            {
                .rtv = renderTarget->GetRtv(),
                .loadOp = RHI::AttachmentLoadOp::Clear,
                .clearValue = { 0, 0, 0, 0 }
            },
            RGDepthStencilAttachment
            {
                .dsv = depthBuffer->GetDsv(),
                .loadOp = RHI::AttachmentLoadOp::Clear,
                .clearValue = { 1, 0 }
            },
            [this]
            {
                RtrcShader::VisualizeHeat::Pass passData;
                passData.worldToClip = camera_.GetWorldToClip();
                passData.mode = visualizationMode_;
                auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

                auto &commandBuffer = RGGetCommandBuffer();
                commandBuffer.BindGraphicsPipeline(visualizeHeatPipeline_);
                commandBuffer.BindGraphicsGroups(passGroup);
                commandBuffer.SetVertexBuffer(0, vertexBuffer_, sizeof(Vertex));
                commandBuffer.SetIndexBuffer(indexBuffer_, RHI::IndexFormat::UInt32);
                commandBuffer.DrawIndexed(indexBuffer_->GetSize() / sizeof(uint32_t), 1, 0, 0, 0);
            });
    }

    struct Vertex
    {
        Vector3f position;
        float    normalizedHeat;
        Vector3f gradient;
        float    divergence;
        float    distance;
    };

    enum VisualizeMode
    {
        Heat,
        Gradient,
        Divergence,
        Distance
    };

    RawMesh          rawMesh_;
    FlatHalfedgeMesh halfEdgeMesh_;
    TriangleBVH      bvh_;

    Eigen::SimplicialLDLT<SparseMatrixXd> dirichletHeatSolver_;
    Eigen::SimplicialLDLT<SparseMatrixXd> neumannHeatSolver_;
    SparseMatrixXd gradientOperatorX_;
    SparseMatrixXd gradientOperatorY_;
    SparseMatrixXd gradientOperatorZ_;
    SparseMatrixXd divergenceOperator_;
    Eigen::SimplicialLDLT<SparseMatrixXd> poissonSolver_;

    RC<Buffer> vertexBuffer_;
    RC<Buffer> indexBuffer_;

    Camera camera_;
    EditorCameraController cameraController_;

    VisualizeMode visualizationMode_ = Distance;

    RC<GraphicsPipeline> visualizeHeatPipeline_;
};

RTRC_APPLICATION_MAIN(
    HeatMethodDemo,
    .title             = "Rtrc Sample: Heat Method",
    .width             = 640,
    .height            = 480,
    .maximized         = true,
    .vsync             = true,
    .debug             = RTRC_DEBUG,
    .rayTracing        = false,
    .backendType       = RHI::BackendType::Default,
    .enableGPUCapturer = false)
