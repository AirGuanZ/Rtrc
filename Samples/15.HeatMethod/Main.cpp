#include <Rtrc/Geometry/DiscreteOperators.h>
#include <Rtrc/Rtrc.h>

#include "Visualize.shader.outh"

using namespace Rtrc;
using namespace Geo;

using VectorXd       = Eigen::VectorXd;
using SparseMatrixXd = Eigen::SparseMatrix<double>;
using SimplicialLDLT = Eigen::SimplicialLDLT<SparseMatrixXd>;

class HeatMethodDemo : public SimpleApplication
{
public:

    using SimpleApplication::SimpleApplication;

    void InitializeSimpleApplication(GraphRef graph) override
    {
        rawMesh_ = RawMesh::Load("./Asset/Sample/15.HeatMethod/Bunny_good.obj");
        rawMesh_.NormalizePositionTo(Vector3f(-1), Vector3f(+1));

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

        lastVs_ = { sourceVertex };
        ComputeGeodesicDistance(false);
    }

    void UpdateSimpleApplication(GraphRef graph) override
    {
        if(input_->IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        if(imgui_->Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            int mode = visualizationMode_;
            if(imgui_->Combo("Visualization Mode", &mode, { "Heat", "Gradient", "Divergence", "Source", "UV", "Distance" }))
            {
                visualizationMode_ = static_cast<VisualizeMode>(mode);
            }
            if(imgui_->Button("Evaluate Source"))
            {
                isDirty_ = true;
                ComputeGeodesicDistance(true);
            }
            if(imgui_->Button("Clear"))
            {
                lastVs_.clear();
                isDirty_ = true;
            }
            imgui_->CheckBox("Append", &append_);
        }
        imgui_->End();

        camera_.SetAspectRatio(window_->GetFramebufferWOverH());
        if(!imgui_->IsAnyItemActive())
        {
            cameraController_.Update(*input_, GetFrameTimer().GetDeltaSecondsF());
            camera_.UpdateDerivedData();
        }

        if(!imgui_->IsAnyItemActive() &&
           !input_->IsKeyPressed(KeyCode::LeftAlt) &&
           input_->IsKeyPressed(KeyCode::MouseLeft))
        {
            const float tx = input_->GetCursorAbsolutePositionX() / window_->GetFramebufferSize().x;
            const float ty = input_->GetCursorAbsolutePositionY() / window_->GetFramebufferSize().y;
            auto worldRays = camera_.GetWorldRays();
            const Vector3f direction =
                Normalize(Lerp(
                    Lerp(worldRays[0], worldRays[1], tx),
                    Lerp(worldRays[2], worldRays[3], tx),
                    ty));
            const Vector3f origin = camera_.GetPosition();

            TriangleBVH<float>::RayIntersectionResult intersection;
            if(bvh_.FindClosestRayIntersection(origin, direction, 0, FLT_MAX, intersection))
            {
                float bestDistance = FLT_MAX; int bestV = -1;
                for(int i = 0; i < 3; ++i)
                {
                    const int v = halfEdgeMesh_.Vert(3 * intersection.triangleIndex + i);
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
                    if(append_)
                    {
                        isDirty_ |= lastVs_.insert(bestV).second;
                    }
                    else
                    {
                        isDirty_ |= lastVs_ != std::set{ bestV };
                        lastVs_ = { bestV };
                    }
                    ComputeGeodesicDistance(false);
                }
            }
        }

        auto framebuffer = graph->RegisterSwapchainTexture(GetSwapchain());
        VisualizeHeat(graph, framebuffer);
    }

    void PreprocessMesh()
    {
        LogInfo("#V = {}", rawMesh_.GetPositionData().size());
        LogInfo("#F = {}", rawMesh_.GetPositionIndices().size() / 3);

        LogInfo("Build BVH");

        bvh_ = TriangleBVH<float>::Build(IndexedPositions<float>(rawMesh_.GetPositionData(), rawMesh_.GetPositionIndices()));

        LogInfo("Build connectivity");

        halfEdgeMesh_ = HalfedgeMesh::Build(rawMesh_.GetPositionIndices());

        auto &m = halfEdgeMesh_;

        LogInfo("Compute time step");

        positions_ =
            rawMesh_.GetPositionData() |
            std::views::transform([](const Vector3f &p) { return p.To<double>(); }) |
            std::ranges::to<std::vector<Vector3d>>();

        double dt;
        {
            double sumEdgeLength = 0;
            for(int e = 0; e < m.E(); ++e)
            {
                const int h = m.EdgeToHalfedge(e);
                const int v0 = m.Head(h);
                const int v1 = m.Tail(h);
                const Vector3d p0 = positions_[v0];
                const Vector3d p1 = positions_[v1];
                sumEdgeLength += Length(p0 - p1);
            }
            const double meanEdgeLength = sumEdgeLength / m.E();
            dt = meanEdgeLength * meanEdgeLength;
        }

        LogInfo("Time step = {}", dt);

        LogInfo("Build area matrix");

        A_ = (1.0 / 3.0) * BuildVertexAreaDiagonalMatrix<double>(m, positions_);

        LogInfo("Prefactorize heat solver with Dirichlet boundary conditions");

        const SparseMatrixXd Ld = BuildCotanLaplacianMatrix<double>(m, positions_, CotanLaplacianBoundaryType::Dirichlet);
        dirichletHeatSolver_.compute(A_ - dt * Ld);

        LogInfo("Prefactorize heat solver with Neumann boundary conditions");

        const SparseMatrixXd Ln = BuildCotanLaplacianMatrix<double>(m, positions_, CotanLaplacianBoundaryType::Neumann);
        neumannHeatSolver_.compute(A_ - dt * Ln);

        LogInfo("Build gradient operator");

        gradientOperatorX_ = BuildFaceGradientMatrix_X<double>(m, positions_);
        gradientOperatorY_ = BuildFaceGradientMatrix_Y<double>(m, positions_);
        gradientOperatorZ_ = BuildFaceGradientMatrix_Z<double>(m, positions_);

        LogInfo("Build divergence operator");

        divergenceOperator_ = BuildVertexDivergenceMatrix<double>(m, positions_);
        normalizedDivergenceOperator_ = BuildVertexDivergenceMatrix_NormalizedByBoundaryLength<double>(m, positions_);

        LogInfo("Prefactorize poisson solver");

        poissonSolver_.compute(Ln);

        LogInfo("Complete preprocessing");
    }

    void ComputeGeodesicDistance(bool evalSourceParameter)
    {
        if(!isDirty_ || lastVs_.empty())
        {
            return;
        }
        isDirty_ = false;

        auto &m = halfEdgeMesh_;

        Timer timer;

        // Heat diffusion

        VectorXd delta = VectorXd::Zero(m.V());
        for(int v : lastVs_)
        {
            delta(v) = 1.0;
        }

        const VectorXd ud = dirichletHeatSolver_.solve(A_ * delta);
        const VectorXd un = neumannHeatSolver_.solve(A_ * delta);
        const VectorXd u = 0.5 * (ud + un);

        // Gradient direction

        const VectorXd gradientX = -gradientOperatorX_ * u;
        const VectorXd gradientY = -gradientOperatorY_ * u;
        const VectorXd gradientZ = -gradientOperatorZ_ * u;
        
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

        flattenHeatGradient_ = VectorXd(m.F() * 3);
        for(int f = 0; f < m.F(); ++f)
        {
            flattenHeatGradient_[3 * f + 0] = gradients[f].x;
            flattenHeatGradient_[3 * f + 1] = gradients[f].y;
            flattenHeatGradient_[3 * f + 2] = gradients[f].z;
        }
        const VectorXd divergence = divergenceOperator_ * flattenHeatGradient_;

        // Poisson

        const VectorXd phi = poissonSolver_.solve(divergence);

        // Geodesic distances

        const double minPhi = *std::ranges::fold_left_first(phi, MinOperator{});
        std::vector<double> geodesicDistances = { phi.begin(), phi.end() };
        for(double &d : geodesicDistances)
        {
            d -= minPhi;
        }

        timer.BeginFrame();
        LogInfo("Complete solving. Delta time = {}ms", timer.GetDeltaSeconds() * 1000);

        // Source parameter

        VectorXd sourceParameters;
        if(evalSourceParameter)
        {
            Vector3d sourceLower(DBL_MAX), sourceUpper(-DBL_MAX);
            for(int v = 0; v < halfEdgeMesh_.V(); ++v)
            {
                if(delta[v] > 0)
                {
                    sourceLower = Min(sourceLower, positions_[v]);
                    sourceUpper = Max(sourceUpper, positions_[v]);
                }
            }
            const Vector3d sourceExtent = sourceUpper - sourceLower;
            const int sourceAxis = ArgMax(sourceExtent.x, sourceExtent.y, sourceExtent.z);

            sourceParameters = -1.0 * VectorXd::Ones(halfEdgeMesh_.V());
            for(int v = 0; v < halfEdgeMesh_.V(); ++v)
            {
                if(delta[v] > 0)
                {
                    const Vector3d p = positions_[v];
                    sourceParameters[v] = Saturate(
                        (p[sourceAxis] - sourceLower[sourceAxis]) / (std::max)(sourceExtent[sourceAxis], 1e-5));
                }
            }
            sourceParameters = SolveSourceParameter(positions_, flattenHeatGradient_, sourceParameters);
        }

        // Visualization

        const double minHeat = *std::ranges::fold_left_first(u, MinOperator{});
        const double maxHeat = *std::ranges::fold_left_first(u, MaxOperator{});

        const double minDivergence = *std::ranges::fold_left_first(divergence, MinOperator{});
        const double maxDivergence = *std::ranges::fold_left_first(divergence, MaxOperator{});
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
            vertex.source = evalSourceParameter ? static_cast<float>(sourceParameters[v]) : 0.0f;
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

    static Vector3d BuildBarycentricGradient(const Vector3d &a, const Vector3d &b, const Vector3d &c)
    {
        const Vector3d nbc = Normalize(b - c);
        const Vector3d direction = Normalize((a - b) - Dot(a - b, nbc) * nbc);
        const double length = Length(b - c) / Length(Cross(b - a, c - a));
        return length * direction;
    }

    VectorXd SolveSourceParameter(
        Span<Vector3d>  positions,
        const VectorXd &binormals,  // per-face
        const VectorXd &sourceValue // per-vertex, -1 means nil
    ) const
    {
        const int knownVertexCount = *std::ranges::fold_left_first(
            sourceValue | std::views::transform([&](double v) { return v >= 0.0 ? 1 : 0; }), std::plus{});
        const int unknownVertexCount = halfEdgeMesh_.V() - knownVertexCount;

        if(halfEdgeMesh_.F() < unknownVertexCount)
        {
            LogInfo("Characteristics equations are underdetermined");
            LogInfo("Unknown vertex count = {}", unknownVertexCount);
            LogInfo("Vertex count = {}", halfEdgeMesh_.V());
            LogInfo("Face count = {}", halfEdgeMesh_.F());
            return {};
        }

        std::vector<int> vertexMap;
        vertexMap.resize(halfEdgeMesh_.V(), -1);
        for(int v = 0, nextMappedVertex = 0; v < halfEdgeMesh_.V(); ++v)
        {
            if(sourceValue[v] < 0)
            {
                vertexMap[v] = nextMappedVertex++;
            }
        }

        std::vector<Eigen::Triplet<double>> triplets;
        std::vector<double> bValues;
        for(int f = 0; f < halfEdgeMesh_.F(); ++f)
        {
            // Try to start from the first unconstrained vertex
            int startIndex = 0;
            for(int i = 0; i < 3; ++i)
            {
                if(sourceValue[halfEdgeMesh_.Vert(3 * f + i)] >= 0.0 &&
                   sourceValue[halfEdgeMesh_.Vert(3 * f + ((i + 1) % 3))] < 0)
                {
                    startIndex = (i + 1) % 3;
                    break;
                }
            }
            auto MI = [&](int i) { return (startIndex + i) % 3; };

            const bool isConstrained[3] =
            {
                sourceValue[halfEdgeMesh_.Vert(3 * f + MI(0))] >= 0.0,
                sourceValue[halfEdgeMesh_.Vert(3 * f + MI(1))] >= 0.0,
                sourceValue[halfEdgeMesh_.Vert(3 * f + MI(2))] >= 0.0,
            };
            if(isConstrained[0])
            {
                // All three source values have been specified. No unknown vertex is related to this face.
                continue;
            }

            const int v[3] =
            {
                halfEdgeMesh_.Vert(3 * f + MI(0)),
                halfEdgeMesh_.Vert(3 * f + MI(1)),
                halfEdgeMesh_.Vert(3 * f + MI(2)),
            };
            const Vector3d p[3] =
            {
                positions[v[0]],
                positions[v[1]],
                positions[v[2]],
            };
            const Vector3d B[3] =
            {
                BuildBarycentricGradient(p[0], p[1], p[2]),
                BuildBarycentricGradient(p[1], p[2], p[0]),
                BuildBarycentricGradient(p[2], p[0], p[1]),
            };
            const double u[3] =
            {
                sourceValue[v[0]],
                sourceValue[v[1]],
                sourceValue[v[2]],
            };

            const Vector3d G =
            {
                binormals[3 * f + 0],
                binormals[3 * f + 1],
                binormals[3 * f + 2]
            };

            const double coef[3] =
            {
                Dot(B[0], G),
                Dot(B[1], G),
                Dot(B[2], G),
            };

            const int nextEquationIndex = static_cast<int>(bValues.size());
            if(isConstrained[1]) // Only u0 is unknown. The gradient is u0 * B0 + u1 * B1 + u2 * B2
            {
                if(std::abs(coef[0]) < 1e-9)
                {
                    continue;
                }
                triplets.emplace_back(nextEquationIndex, vertexMap[v[0]], coef[0]);
                bValues.push_back(-(u[1] * coef[1] + u[2] * coef[2]));
            }
            else if(isConstrained[2]) // Both u0 and u1 are unknown
            {
                if(std::abs(coef[0]) < 1e-9 && std::abs(coef[1]) < 1e-9)
                {
                    continue;
                }
                triplets.emplace_back(nextEquationIndex, vertexMap[v[0]], coef[0]);
                triplets.emplace_back(nextEquationIndex, vertexMap[v[1]], coef[1]);
                bValues.push_back(-(u[2] * coef[2]));
            }
            else // All three vertices are unknown
            {
                if(std::abs(coef[0]) < 1e-9 && std::abs(coef[1]) < 1e-9 && std::abs(coef[2]) < 1e-9)
                {
                    continue;
                }
                triplets.emplace_back(nextEquationIndex, vertexMap[v[0]], coef[0]);
                triplets.emplace_back(nextEquationIndex, vertexMap[v[1]], coef[1]);
                triplets.emplace_back(nextEquationIndex, vertexMap[v[2]], coef[2]);
                bValues.push_back(0);
            }
        }

        SparseMatrixXd A(bValues.size(), unknownVertexCount);
        A.setFromTriplets(triplets.begin(), triplets.end());

        VectorXd b(bValues.size());
        for(uint32_t i = 0; i < bValues.size(); ++i)
        {
            b[i] = bValues[i];
        }

        Eigen::LeastSquaresConjugateGradient<SparseMatrixXd> CG;
        CG.compute(A);
        const VectorXd u = CG.solve(b);

        VectorXd remappedU(halfEdgeMesh_.V());
        for(int v = 0; v < halfEdgeMesh_.V(); ++v)
        {
            if(sourceValue[v] >= 0)
            {
                remappedU[v] = sourceValue[v];
            }
            else
            {
                remappedU[v] = u[vertexMap[v]];
            }
        }

        return remappedU;
    }

    void VisualizeHeat(GraphRef graph, RGTexture renderTarget)
    {
        auto depthBuffer = graph->CreateTexture(RHI::TextureDesc
        {
            .format     = RHI::Format::D32,
            .width      = renderTarget->GetWidth(),
            .height     = renderTarget->GetHeight(),
            .usage      = RHI::TextureUsage::DepthStencil,
            .clearValue = RHI::DepthStencilClearValue{ 1, 0 }
        });

        if(!visualizeHeatPipeline_)
        {
            visualizeHeatPipeline_ = device_->CreateGraphicsPipeline(GraphicsPipelineDesc
            {
                .shader = device_->GetShader<RtrcShader::VisualizeHeat::Name>(),
                .meshLayout = RTRC_MESH_LAYOUT(Buffer(
                    Attribute("POSITION",   Float3),
                    Attribute("HEAT",       Float),
                    Attribute("GRADIENT",   Float3),
                    Attribute("DIVERGENCE", Float),
                    Attribute("SOURCE",     Float),
                    Attribute("DISTANCE",   Float))),
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
        float    source;
        float    distance;
    };

    enum VisualizeMode
    {
        Heat,
        Gradient,
        Divergence,
        Source,
        UV,
        Distance
    };

    RawMesh               rawMesh_;
    HalfedgeMesh          halfEdgeMesh_;
    TriangleBVH<float>    bvh_;
    std::vector<Vector3d> positions_;

    SparseMatrixXd A_;
    SimplicialLDLT dirichletHeatSolver_;
    SimplicialLDLT neumannHeatSolver_;
    SparseMatrixXd gradientOperatorX_;
    SparseMatrixXd gradientOperatorY_;
    SparseMatrixXd gradientOperatorZ_;
    SparseMatrixXd divergenceOperator_;
    SparseMatrixXd normalizedDivergenceOperator_;
    SimplicialLDLT poissonSolver_;

    VectorXd flattenHeatGradient_;

    bool append_ = false;
    bool isDirty_ = true;
    std::set<int> lastVs_;

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
