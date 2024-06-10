#include <Rtrc/Rtrc.h>

#include "Visualize.shader.outh"

using namespace Rtrc;

struct Mesh
{
    std::vector<Vector3f> positions;
    std::vector<Vector3f> normals;
    std::vector<uint32_t> indices;

    Vector3f lower;
    Vector3f upper;

    RC<Buffer> positionBuffer;
    RC<Buffer> normalBuffer;
    RC<Buffer> indexBuffer;
};

Mesh LoadMeshFromObj(DeviceRef device, const std::string &filename)
{
    auto meshData = MeshData::LoadFromObjFile(filename, true, false);

    Mesh mesh;
    mesh.positions = std::move(meshData.positionData);
    mesh.normals = std::move(meshData.normalData);
    mesh.indices = std::move(meshData.indexData);

    Vector3f lower(FLT_MAX), upper(-FLT_MAX);
    for(auto &p : mesh.positions)
    {
        lower = Min(lower, p);
        upper = Max(upper, p);
    }
    const Vector3f extent = Max(upper - lower, Vector3f(0.01f));
    for(auto &p : mesh.positions)
    {
        p = (p - lower) / extent;
        p = 2.0f * p - Vector3f(1.0f);
    }
    mesh.lower = { -1, -1, -1 };
    mesh.upper = { +1, +1, +1 };

    mesh.positionBuffer = device->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size = mesh.positions.size() * sizeof(Vector3f),
        .usage = RHI::BufferUsage::VertexBuffer
    }, mesh.positions.data());
    mesh.normalBuffer = device->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size = mesh.normals.size() * sizeof(Vector3f),
        .usage = RHI::BufferUsage::VertexBuffer
    }, mesh.normals.data());
    mesh.indexBuffer = device->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size = mesh.indices.size() * sizeof(uint32_t),
        .usage = RHI::BufferUsage::IndexBuffer
    }, mesh.indices.data());

    return mesh;
}

void VoxelizeWithDilation(
    const Mesh    &mesh,
    uint32_t       resolution,
    Image3D<bool> &voxels,
    Vector3f      &volumeLower,
    Vector3f      &volumeUpper)
{
    assert(resolution >= 5);

    // Compute mesh bounds and initialize voxels

    const Vector3f meshExtent = mesh.upper - mesh.lower;
    const float maxExtent = MaxReduce(meshExtent);
    const float voxelSize = maxExtent / (resolution - 4.0f);

    const Vector3u dims = Min(Ceil(meshExtent / voxelSize).To<uint32_t>() + Vector3u(4), Vector3u(resolution));
    const Vector3f volumeExtent = voxelSize * dims.To<float>();
    const Vector3f lower = 0.5f * (mesh.lower + mesh.upper) - 0.5f * volumeExtent;

    voxels = Image3D<bool>(dims);
    std::fill(voxels.begin(), voxels.end(), false);

    volumeLower = lower;
    volumeUpper = lower + volumeExtent;

    // Traversal triangles to fill voxels

    const float rcpVoxelSize = 1.0f / voxelSize;
    auto PositionToVoxel = [&](const Vector3f& p)
    {
        return Floor(rcpVoxelSize * (p - lower)).To<uint32_t>();
    };

    assert(mesh.indices.size() % 3 == 0);
    for(unsigned ii = 0; ii < mesh.indices.size(); ii += 3)
    {
        const uint32_t i0 = mesh.indices[ii + 0];
        const uint32_t i1 = mesh.indices[ii + 1];
        const uint32_t i2 = mesh.indices[ii + 2];

        const Vector3f &p0 = mesh.positions[i0];
        const Vector3f &p1 = mesh.positions[i1];
        const Vector3f &p2 = mesh.positions[i2];

        const Vector3f pMin = Min(p0, Min(p1, p2));
        const Vector3f pMax = Max(p0, Max(p1, p2));

        const Vector3u vMin = PositionToVoxel(pMin);
        const Vector3u vMax = PositionToVoxel(pMax) + Vector3u(1);

        for(uint32_t z = vMin.z; z <= vMax.z; ++z)
        {
            for(uint32_t y = vMin.y; y <= vMax.y; ++y)
            {
                for(uint32_t x = vMin.x; x <= vMax.x; ++x)
                {
                    const Vector3u voxel = { x, y, z };
                    const Vector3f voxelMin = lower + voxelSize * voxel.To<float>();
                    const Vector3f voxelMax = voxelMin + Vector3f(voxelSize);
                    if(IntersectTriangleAABB(p0, p1, p2, voxelMin, voxelMax))
                    {
                        voxels(x, y, z) = true;
                    }
                }
            }
        }
    }

    auto ForEachNeighbor = [&]<typename F>(const Vector3i &p, const F &func)
    {
        const int x0 = (std::max<int>)(p.x - 1, 0);
        const int x1 = (std::min<int>)(p.x + 1, voxels.GetSize(0) - 1);
        const int y0 = (std::max<int>)(p.y - 1, 0);
        const int y1 = (std::min<int>)(p.y + 1, voxels.GetSize(1) - 1);
        const int z0 = (std::max<int>)(p.z - 1, 0);
        const int z1 = (std::min<int>)(p.z + 1, voxels.GetSize(2) - 1);
        for(int z = z0; z <= z1; ++z)
        {
            for(int y = y0; y <= y1; ++y)
            {
                for(int x = x0; x <= x1; ++x)
                {
                    func(Vector3i(x, y, z));
                }
            }
        }
    };

    for(int i = 0; i < 1; ++i)
    {
        Image3D<bool> voxelsEx = voxels;
        for(uint32_t z = 0; z < voxels.GetDepth(); ++z)
        {
            for(uint32_t y = 0; y < voxels.GetHeight(); ++y)
            {
                for(uint32_t x = 0; x < voxels.GetWidth(); ++x)
                {
                    ForEachNeighbor(Vector3u(x, y, z).To<int>(), [&](const Vector3i &p)
                    {
                        if(voxels(p))
                        {
                            voxelsEx(x, y, z) = true;
                        }
                    });
                }
            }
        }
        voxels = std::move(voxelsEx);
    }
}

struct CompareVoxelUsingDistance
{
    MultiDimSpan<float, 3> distances;

    explicit CompareVoxelUsingDistance(MultiDimSpan<float, 3> distances) : distances(distances) {}

    bool operator()(const Vector3u& lhs, const Vector3u& rhs) const
    {
        const float distLhs = distances(lhs.x, lhs.y, lhs.z);
        const float distRhs = distances(rhs.x, rhs.y, rhs.z);
        return distLhs != distRhs ? (distLhs < distRhs) : (lhs < rhs);
    }
};

void FMM3D(
    const Image3D<bool>         &volume,
    const std::vector<Vector3i> &sources,
    Image3D<float>              &output)
{
    Image3D<bool> isFixed(volume.GetSizes());
    std::fill(isFixed.begin(), isFixed.end(), false);

    assert(output.GetSizes() == volume.GetSizes());
    std::fill(output.begin(), output.end(), FLT_MAX);
    for(auto &s : sources)
    {
        output(s) = 0;
        isFixed(s) = true;
    }
    
    auto Compare = [&](const Vector3i &lhs, const Vector3i &rhs)
    {
        const float distLhs = output(lhs);
        const float distRhs = output(rhs);
        return distLhs != distRhs ? distLhs < distRhs : lhs < rhs;
    };
    std::set<Vector3i, decltype(Compare)> A(Compare);
    
    auto ForEachNeighbors = [&]<typename F>(const Vector3i &p, const F &f)
    {
        auto Access = [&](const Vector3i &n) { if(volume(n)) { f(n); } };
        if(p.x > 0) { Access(Vector3i(p.x - 1, p.y, p.z)); }
        if(p.y > 0) { Access(Vector3i(p.x, p.y - 1, p.z)); }
        if(p.z > 0) { Access(Vector3i(p.x, p.y, p.z - 1)); }
        if(p.x + 1u < output.GetSize(0)) { Access(Vector3i(p.x + 1, p.y, p.z)); }
        if(p.y + 1u < output.GetSize(1)) { Access(Vector3i(p.x, p.y + 1, p.z)); }
        if(p.z + 1u < output.GetSize(2)) { Access(Vector3i(p.x, p.y, p.z + 1)); }
    };
    
    for(auto &s : sources)
    {
        ForEachNeighbors(s, [&](const Vector3i &p)
        {
            if(output(p) == FLT_MAX)
            {
                output(p) = 1;
                A.insert(p);
            }
        });
    }

    auto UpdateT = [&](const Vector3i &p)
    {
        float nx = FLT_MAX;
        if(p.x > 0) { nx = (std::min)(nx, output(p.x - 1, p.y, p.z)); }
        if(p.x + 1u < output.GetSize(0)) { nx = (std::min)(nx, output(p.x + 1, p.y, p.z)); }
        float ny = FLT_MAX;
        if(p.y > 0) { ny = (std::min)(ny, output(p.x, p.y - 1, p.z)); }
        if(p.y + 1u < output.GetSize(1)) { ny = (std::min)(ny, output(p.x, p.y + 1, p.z)); }
        float nz = FLT_MAX;
        if(p.z > 0) { nz = (std::min)(nz, output(p.x, p.y, p.z - 1)); }
        if(p.z + 1u < output.GetSize(2)) { nz = (std::min)(nz, output(p.x, p.y, p.z + 1)); }
        assert(nx != FLT_MAX || ny != FLT_MAX || nz != FLT_MAX);

        const float min = (std::min)({ nx, ny, nz });
        const float max = min + 1;
        StaticVector<float, 3> neis;
        if(nx < max) { neis.PushBack(nx); }
        if(ny < max) { neis.PushBack(ny); }
        if(nz < max) { neis.PushBack(nz); }
        if(neis.GetSize() == 1)
        {
            return min + 1;
        }

        const float a = neis.GetSize();
        float b = 0, c = -1;
        for(float v : neis)
        {
            b += -2 * v;
            c += v * v;
        }

        const float sqrtDelta = std::sqrt(std::max(b * b - 4 * a * c, 0.0f));
        const float t = (-b + sqrtDelta) / (2 * a);
        return t;
    };

    while(!A.empty())
    {
        const Vector3i p = *A.begin();
        A.erase(A.begin());
        isFixed(p) = true;
        
        ForEachNeighbors(p, [&](const Vector3i &n)
        {
            if(isFixed(n))
            {
                return;
            }
            const float oldT = output(n);
            const float newT = UpdateT(n);
            if(newT < oldT)
            {
                if(oldT != FLT_MAX)
                {
                    A.erase(n);
                }
                output(n) = newT;
                A.insert(n);
            }
        });
    }
}

class GeodesicDistanceDemo : public SimpleApplication
{
    Mesh mesh_;

    uint32_t resolution_ = RTRC_DEBUG ? 128 : 256;
    Image3D<bool> voxels_;
    Vector3f volumeLower_;
    Vector3f volumeUpper_;

    Vector3f sourcePosition_;
    Image3D<float> distances_;
    RC<Texture> distanceTexture_;
    float maxDistance_ = 0;
    uint32_t visualizeSegments_ = 35;
    float lineWidthScale_ = 2.5f;

    Camera camera_;
    EditorCameraController cameraController_;

    GraphicsPipelineCache pipelineCache_;

    void InitializeSimpleApplication(GraphRef graph) override
    {
        pipelineCache_.SetDevice(GetDevice());
        LoadMesh("Asset/Sample/11.GeodesicDistance/Bunny.obj");
        Voxelize();
        EvaluateDistanceField();
    }

    void UpdateSimpleApplication(GraphRef graph) override
    {
        if(GetWindowInput().IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        bool isResolutionChanged = false;
        bool isSourcePositionChanged = false;

        auto &imgui = *GetImGuiInstance();
        if(imgui.Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            imgui.BeginDisabled();
            imgui.InputVector3("Min", volumeLower_);
            imgui.InputVector3("Max", volumeUpper_);
            imgui.EndDisabled();
            isSourcePositionChanged = imgui.InputVector3("Source", sourcePosition_);
            isResolutionChanged = imgui.Input("Resolution", &resolution_, nullptr, ImGuiInputTextFlags_EnterReturnsTrue);
            imgui.Slider("Segments", &visualizeSegments_, 1u, 100u);
            imgui.Slider("LineWidthScale", &lineWidthScale_, 0.0f, 5.0f);
            resolution_ = std::clamp(resolution_, 5u, 512u);
        }
        imgui.End();

        if(isResolutionChanged)
        {
            Voxelize();
        }
        if(isResolutionChanged || isSourcePositionChanged)
        {
            EvaluateDistanceField();
        }

        camera_.SetAspectRatio(GetWindow().GetFramebufferWOverH());
        cameraController_.Update(GetWindowInput(), GetFrameTimer().GetDeltaSecondsF());
        camera_.UpdateDerivedData();

        auto swapchainTexture = graph->RegisterSwapchainTexture(GetDevice()->GetSwapchain());
        RGClearColor(graph, "ClearSwapchainTexture", swapchainTexture, { 0, 1, 1, 0 });

        auto depthBuffer = graph->CreateTexture(RHI::TextureDesc
        {
            .format = RHI::Format::D32,
            .width = swapchainTexture->GetWidth(),
            .height = swapchainTexture->GetHeight(),
            .usage = RHI::TextureUsage::DepthStencil | RHI::TextureUsage::ClearDst,
            .clearValue = RHI::DepthStencilClearValue{ 1, 0 }
        });
        RGClearDepthStencil(graph, "ClearDepth", depthBuffer, 1, 0);

        Render(graph, swapchainTexture, depthBuffer);
    }

    void LoadMesh(const std::string &filename)
    {
        mesh_ = LoadMeshFromObj(GetDevice(), filename);

        const Vector3f center = 0.5f * (mesh_.lower + mesh_.upper);
        const Vector3f extent = mesh_.upper - mesh_.lower;
        camera_.SetLookAt(center + 0.7f * extent, { 0, 1, 0 }, center);
        cameraController_.SetCamera(camera_);
        cameraController_.SetTrackballDistance(Length(camera_.GetPosition() - center));
        camera_.UpdateDerivedData();
    }

    void Voxelize()
    {
        VoxelizeWithDilation(mesh_, resolution_, voxels_, volumeLower_, volumeUpper_);
        sourcePosition_ = volumeUpper_;
    }

    void EvaluateDistanceField()
    {
        // Find source voxel

        const Vector3f st = (sourcePosition_ - volumeLower_) / (volumeUpper_ - volumeLower_);
        const Vector3f sp = st * Vector3f(voxels_.GetSize(0), voxels_.GetSize(1), voxels_.GetSize(2)) - Vector3f(0.5f);

        Vector3i sourceVoxel; float minDist = FLT_MAX;
        for(uint32_t z = 0; z < voxels_.GetDepth(); ++z)
        {
            for(uint32_t y = 0; y < voxels_.GetHeight(); ++y)
            {
                for(uint32_t x = 0; x < voxels_.GetWidth(); ++x)
                {
                    if(!voxels_(x, y, z))
                    {
                        continue;
                    }
                    const float dist = Length(Vector3f(x, y, z) - sp);
                    if(dist < minDist)
                    {
                        sourceVoxel = Vector3u(x, y, z).To<int>();
                        minDist = dist;
                    }
                }
            }
        }

        // Run FMM

        distances_ = Image3D<float>(voxels_.GetSizes());
        FMM3D(voxels_, { sourceVoxel }, distances_);

        for(float &d : distances_)
        {
            if(d == FLT_MAX)
            {
                d = 0;
            }
        }

        // Upload distance texture

        distanceTexture_= GetDevice()->CreateAndUploadTexture(RHI::TextureDesc
        {
            .dim    = RHI::TextureDimension::Tex3D,
            .format = RHI::Format::R32_Float,
            .width  = distances_.GetWidth(),
            .height = distances_.GetHeight(),
            .depth  = distances_.GetDepth(),
            .usage  = RHI::TextureUsage::ShaderResource
        }, distances_.GetData(), RHI::TextureLayout::ShaderTexture);

        // Compute max distance

        maxDistance_ = 0;
        for(float d : distances_)
        {
            maxDistance_ = (std::max)(maxDistance_, d);
        }
    }

    void Render(GraphRef graph, RGTexture framebuffer, RGTexture depthBuffer)
    {
        using Shader = RtrcShader::VisualizeMeshWithDistanceField;

        RGAddRenderPass<true>(
            graph, "RenderMesh",
            RGColorAttachment
            {
                .rtv = framebuffer->GetRtv(),
                .isWriteOnly = true,
            },
            RGDepthStencilAttachment
            {
                .dsv = depthBuffer->GetDsv()
            }, [this, framebuffer, depthBuffer]
            {
                auto &commandBuffer = RGGetCommandBuffer();

                Shader::Pass passData;
                passData.DistanceTexture    = distanceTexture_;
                passData.lineWidthScale     = lineWidthScale_;
                passData.visualizeSegments  = visualizeSegments_;
                passData.distanceScale      = 1.0f / std::max(0.01f, maxDistance_);
                passData.worldToClip        = camera_.GetWorldToClip();
                passData.positionToUVWScale = Vector3f(1) / (volumeUpper_ - volumeLower_);
                passData.positionToUVWBias  = -passData.positionToUVWScale * volumeLower_;
                auto passGroup = GetDevice()->CreateBindingGroupWithCachedLayout(passData);

                auto pipeline = pipelineCache_.Get(GraphicsPipelineDesc
                {
                    .shader = GetDevice()->GetShader<Shader::Name>(),
                    .meshLayout = RTRC_MESH_LAYOUT(
                                    Buffer(Attribute("POSITION", Float3)),
                                    Buffer(Attribute("NORMAL", Float3))),
                    .depthStencilState = RTRC_STATIC_DEPTH_STENCIL_STATE(
                    {
                        .enableDepthTest = true,
                        .enableDepthWrite = true,
                        .depthCompareOp = RHI::CompareOp::LessEqual
                    }),
                    .attachmentState = RTRC_ATTACHMENT_STATE
                    {
                        .colorAttachmentFormats = { framebuffer->GetFormat() },
                        .depthStencilFormat = depthBuffer->GetFormat()
                    }
                });
                commandBuffer.BindGraphicsPipeline(pipeline);
                commandBuffer.BindGraphicsGroups(passGroup);
                commandBuffer.SetVertexBuffers(
                    0, { mesh_.positionBuffer, mesh_.normalBuffer },
                    { sizeof(Vector3f), sizeof(Vector3f) });
                commandBuffer.SetIndexBuffer(mesh_.indexBuffer, RHI::IndexFormat::UInt32);
                commandBuffer.DrawIndexed(mesh_.indices.size(), 1, 0, 0, 0);
            });
    }
};

RTRC_APPLICATION_MAIN(
    GeodesicDistanceDemo,
    .title             = "Rtrc Sample: Geodesic Distance",
    .width             = 640,
    .height            = 480,
    .maximized         = true,
    .vsync             = true,
    .debug             = RTRC_DEBUG,
    .rayTracing        = false,
    .backendType       = RHI::BackendType::Default,
    .enableGPUCapturer = false)
