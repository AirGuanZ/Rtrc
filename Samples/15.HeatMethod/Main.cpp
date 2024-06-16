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

        const std::vector<Vector3d> positions =
            rawMesh_.GetPositionData() |
            std::views::transform([](const Vector3f &p) { return p.To<double>(); }) |
            std::ranges::to<std::vector<Vector3d>>();
        
        auto m = FlatHalfedgeMesh::Build(rawMesh_.GetPositionIndices());

        // Compute time step

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

        // Heat diffusion

        const SparseMatrixXd A = (1.0 / 3.0) * BuildVertexAreaDiagonalMatrix<double>(m, positions);
        const SparseMatrixXd Ld = BuildCotanLaplacianMatrix<double>(m, positions, CotanLaplacianBoundaryType::Dirichlet);
        const SparseMatrixXd Ln = BuildCotanLaplacianMatrix<double>(m, positions, CotanLaplacianBoundaryType::Neumann);

        Eigen::VectorXd delta = Eigen::VectorXd::Zero(m.V());
        for(int v = 0; v < m.V(); ++v)
        {
            if(!m.IsVertOnBoundary(v)) // Set the first non-boundary vertex as heat source
            {
                delta(v) = 1.0;
                break;
            }
        }

        Eigen::SimplicialLDLT<SparseMatrixXd> dirichletHeatSolver;
        dirichletHeatSolver.compute(A - dt * Ld);
        const Eigen::VectorXd ud = dirichletHeatSolver.solve(delta);

        Eigen::SimplicialLDLT<SparseMatrixXd> neumannHeatSolver;
        neumannHeatSolver.compute(A - dt * Ln);
        const Eigen::VectorXd un = neumannHeatSolver.solve(delta);

        const Eigen::VectorXd u = 0.5 * (ud + un);

        // Gradient direction

        const SparseMatrixXd GX = BuildFaceGradientMatrix_X<double>(m, positions);
        const SparseMatrixXd GY = BuildFaceGradientMatrix_Y<double>(m, positions);
        const SparseMatrixXd GZ = BuildFaceGradientMatrix_Z<double>(m, positions);

        const Eigen::VectorXd gradientX = -GX * u;
        const Eigen::VectorXd gradientY = -GY * u;
        const Eigen::VectorXd gradientZ = -GZ * u;

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

        const SparseMatrixXd Div = BuildVertexDivergenceMatrix<double>(m, positions);
        const Eigen::VectorXd divergence = Div * flattenGradients;

        // Poisson

        Eigen::SimplicialLDLT<SparseMatrixXd> poissonSolver;
        poissonSolver.compute(Ln);
        const Eigen::VectorXd phi = poissonSolver.solve(divergence);

        // Geodesic distances

        const double minPhi = *std::ranges::fold_left_first(phi, [](double a, double b) { return (std::min)(a, b); });
        std::vector<double> geodesicDistances = { phi.begin(), phi.end() };
        for(double &d : geodesicDistances)
        {
            d -= minPhi;
        }

        // Normalize for visualization

        const double minHeat = *std::ranges::fold_left_first(u, [](double a, double b) { return (std::min)(a, b); });
        const double maxHeat = *std::ranges::fold_left_first(u, [](double a, double b) { return (std::max)(a, b); });
        LogInfo("Min heat = {}, max heat = {}", minHeat, maxHeat);

        const double minDivergence = *std::ranges::fold_left_first(divergence, [](double a, double b) { return (std::min)(a, b); });
        const double maxDivergence = *std::ranges::fold_left_first(divergence, [](double a, double b) { return (std::max)(a, b); });
        LogInfo("Min divergence = {}, max divergence = {}", minDivergence, maxDivergence);
        const double maxAbsDivergence = (std::max)(std::abs(minDivergence), std::abs(maxDivergence));

        // Transfer gradients to vertices for visualization

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
                const Vector3d p0 = positions[m.Vert(h0)];
                const Vector3d p1 = positions[m.Vert(h1)];
                const Vector3d p2 = positions[m.Vert(h2)];
                const double cosAngle = Cos(p1 - p0, p2 - p0);
                const double angle = std::acos(Clamp(cosAngle, -1.0, 1.0));
                sumWeightedGradients += angle * gradients[f];
                sumWeights += angle;
            });
            vertexGradients[v] = NormalizeIfNotZero(sumWeightedGradients).To<float>();
        }

        // Build render data

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

        auto framebuffer = graph->RegisterSwapchainTexture(GetSwapchain());
        VisualizeHeat(graph, framebuffer);
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
        float normalizedHeat;
        Vector3f gradient;
        float divergence;
        float distance;
    };

    enum VisualizeMode
    {
        Heat,
        Gradient,
        Divergence,
        Distance
    };

    RawMesh rawMesh_;

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
