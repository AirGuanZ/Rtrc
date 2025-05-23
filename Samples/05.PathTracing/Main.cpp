#include <Rtrc/Rtrc.h>

using namespace Rtrc;

#include "InitRng.shader.outh"
#include "PathTracing.shader.outh"

struct Vertex
{
    Vector3f position;
};

struct Primitive
{
    Vector3f a;
    Vector3f b;
    Vector3f c;
    Vector3f normal;
};

struct Object
{
    RC<Blas>   blas;
    RC<Buffer> primitives; // StructuredBuffer<Primitive>
};

struct SurfaceMaterial
{
    Vector3f color;
};

Object LoadObject(Device &device, const std::string &filename)
{
    const MeshData meshData = MeshData::LoadFromObjFile(filename);

    auto vertexBuffer = device.CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size           = sizeof(Vector3f) * meshData.positionData.size(),
        .usage          = RHI::BufferUsage::AccelerationStructureBuildInput | RHI::BufferUsage::DeviceAddress,
        .hostAccessType = RHI::BufferHostAccessType::None
    }, meshData.positionData.data());
    vertexBuffer->SetName("VertexBuffer");
    
    auto indexBuffer = device.CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size           = sizeof(uint32_t) * meshData.indexData.size(),
        .usage          = RHI::BufferUsage::AccelerationStructureBuildInput | RHI::BufferUsage::DeviceAddress,
        .hostAccessType = RHI::BufferHostAccessType::None
    }, meshData.indexData.data());
    indexBuffer->SetName("IndexBuffer");

    Object ret;
    ret.blas = device.CreateBlas();
    device.ExecuteAndWait([&](CommandBuffer &commandBuffer)
    {
        const RHI::RayTracingGeometryDesc geometry =
        {
            .type           = RHI::RayTracingGeometryType::Triangles,
            .primitiveCount = static_cast<uint32_t>(meshData.indexData.size() / 3),
            .opaque         = true,
            .trianglesData  = RHI::RayTracingTrianglesGeometryData
            {
                .vertexFormat = RHI::RayTracingVertexFormat::Float3,
                .vertexStride = sizeof(Vector3f),
                .vertexCount  = static_cast<uint32_t>(meshData.positionData.size()),
                .indexFormat  = RHI::RayTracingIndexFormat::UInt32,
                .hasTransform = false,
                .vertexData   = vertexBuffer->GetDeviceAddress(),
                .indexData    = indexBuffer->GetDeviceAddress()
            }
        };
        commandBuffer.BuildBlas(
            ret.blas, geometry, RHI::RayTracingAccelerationStructureBuildFlagBit::PreferFastTrace, nullptr);
    });

    std::vector<Primitive> primitives;
    for(size_t i = 0; i < meshData.indexData.size(); i += 3)
    {
        const uint32_t ia = meshData.indexData[i + 0];
        const uint32_t ib = meshData.indexData[i + 1];
        const uint32_t ic = meshData.indexData[i + 2];

        Primitive primitive;
        primitive.a = meshData.positionData[ia];
        primitive.b = meshData.positionData[ib];
        primitive.c = meshData.positionData[ic];
        primitive.normal = -Normalize(Cross(primitive.c - primitive.a, primitive.b - primitive.a));
        primitives.push_back(primitive);
    }
    ret.primitives = device.CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size           = sizeof(Primitive) * primitives.size(),
        .usage          = RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::TransferDst,
        .hostAccessType = RHI::BufferHostAccessType::None
    }, primitives.data());
    ret.primitives->SetDefaultStructStride(sizeof(Primitive));
    ret.primitives->SetName("PrimitiveInfoBuffer");

    return ret;
}

void Run()
{
    Window window = WindowBuilder()
        .SetSize(800, 800)
        .SetTitle("Rtrc Sample: PathTracing")
        .Create();

    Box<Device> device = Device::CreateGraphicsDevice(
        {
            .window = window,
            .flags = Device::EnableRayTracing
        });

    ResourceManager resourceManager(device);
    
    // Scene

    std::vector<SurfaceMaterial> materialData;
    materialData.push_back({ { 0.7f, 0.7f, 0.7f } });
    materialData.push_back({ { 1.0f, 0.1f, 0.1f } });
    auto materialBuffer = device->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size           = sizeof(SurfaceMaterial) * materialData.size(),
        .usage          = RHI::BufferUsage::ShaderStructuredBuffer,
        .hostAccessType = RHI::BufferHostAccessType::None
    }, materialData.data());
    materialBuffer->SetDefaultStructStride(sizeof(SurfaceMaterial));

    auto room = LoadObject(*device, "Asset/Sample/05.PathTracing/Room.obj");
    auto torus = LoadObject(*device, "Asset/Sample/05.PathTracing/Torus.obj");

    const std::vector instanceData =
    {
        RHI::RayTracingInstanceData
        {
            .transform3x4                 = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 } },
            .instanceCustomIndex          = 0,
            .instanceMask                 = 0xff,
            .instanceSbtOffset            = 0,
            .flags                        = 0,
            .accelerationStructureAddress = room.blas->GetRHIObject()->GetDeviceAddress().address
        },
        RHI::RayTracingInstanceData
        {
            .transform3x4                 = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 } },
            .instanceCustomIndex          = 0,
            .instanceMask                 = 0xff,
            .instanceSbtOffset            = 0,
            .flags                        = 0,
            .accelerationStructureAddress = torus.blas->GetRHIObject()->GetDeviceAddress().address
        },
    };
    auto instanceDataBuffer = device->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size           = sizeof(RHI::RayTracingInstanceData) * instanceData.size(),
        .usage          = RHI::BufferUsage::AccelerationStructureBuildInput | RHI::BufferUsage::DeviceAddress,
        .hostAccessType = RHI::BufferHostAccessType::None
    }, instanceData.data());

    auto tlas = device->CreateTlas();
    device->ExecuteAndWait([&](CommandBuffer &commandBuffer)
    {
        const RHI::RayTracingInstanceArrayDesc instanceArrayDesc =
        {
            .instanceCount = static_cast<uint32_t>(instanceData.size()),
            .instanceData  = instanceDataBuffer->GetDeviceAddress()
        };
        commandBuffer.BuildTlas(
            tlas, instanceArrayDesc, RHI::RayTracingAccelerationStructureBuildFlags::PreferFastTrace, nullptr);
    });

    // Pipeline

    auto traceShader = device->GetShader("PathTracing", true);
    auto tracePipeline = traceShader->GetComputePipeline();

    auto initRngShader = device->GetShader("InitRng", true);
    auto initRngPipeline = initRngShader->GetComputePipeline();

    // Render target

    RC<StatefulTexture> accumulateTexture;
    RC<StatefulTexture> rngTexture;

    // Camera

    Camera camera;
    camera.SetLookAt({ 1.2f, 1.5f, -3 }, { 0, 1, 0 }, { 0, 0, 0 });
    camera.SetFovYDeg(60);

    EditorCameraController cameraController;
    cameraController.SetCamera(&camera);
    cameraController.SetTrackballDistance(Length(camera.GetPosition()));

    // Render loop

    RGExecuter executer(device);

    window.SetFocus();
    
    device->BeginRenderLoop();
    RTRC_SCOPE_EXIT{ device->EndRenderLoop(); };

    Timer timer;
    int fps = 0;

    while(!window.ShouldClose())
    {
        if(!device->BeginFrame())
        {
            continue;
        }

        executer.Recycle();
        
        if(window.GetInput().IsKeyDown(KeyCode::Escape))
        {
            window.SetCloseFlag(true);
        }

        timer.BeginFrame();
        if(timer.GetFps() != fps)
        {
            fps = timer.GetFps();
            window.SetTitle(std::format("Rtrc Sample: PathTracing. FPS: {}", timer.GetFps()));
        }

        camera.SetAspectRatio(window.GetWindowWOverH());
        bool needClear = cameraController.Update(window.GetInput(), timer.GetDeltaSecondsF());
        camera.UpdateDerivedData();

        auto graph = device->CreateRenderGraph();
        auto rgSwapchain = graph->RegisterSwapchainTexture(device->GetSwapchain());
        auto renderTarget = graph->CreateTexture(RHI::TextureDesc
        {
            .format = RHI::Format::B8G8R8A8_UNorm,
            .width  = rgSwapchain->GetWidth(),
            .height = rgSwapchain->GetHeight(),
            .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderedAccess
        });

        if(accumulateTexture && (accumulateTexture->GetSize() != renderTarget->GetSize() || needClear))
        {
            accumulateTexture = nullptr;
        }

        RGTexture rgAccumulateTexture, rgRngTexture;
        RGPass initializePass = graph->CreatePass("Clear history texture");
        if(!accumulateTexture)
        {
            accumulateTexture = device->CreateStatefulTexture(RHI::TextureDesc
            {
                .format = RHI::Format::R32G32B32A32_Float,
                .width  = renderTarget->GetWidth(),
                .height = renderTarget->GetHeight(),
                .usage  = RHI::TextureUsage::ShaderResource |
                          RHI::TextureUsage::UnorderedAccess |
                          RHI::TextureUsage::ClearDst,
            });
            accumulateTexture->SetName("AccumulateTexture");

            rngTexture = device->CreateStatefulTexture(RHI::TextureDesc
            {
                .format = RHI::Format::R32_UInt,
                .width  = renderTarget->GetWidth(),
                .height = renderTarget->GetHeight(),
                .usage  = RHI::TextureUsage::UnorderedAccess,
            });
            rngTexture->SetName("RngTexture");

            rgAccumulateTexture = graph->RegisterTexture(accumulateTexture);
            rgRngTexture = graph->RegisterTexture(rngTexture);

            RGClearRWTexture2D(graph, "ClearAccumulationTexture", rgAccumulateTexture, Vector4f(0));

            initializePass->Use(rgRngTexture, RG::CS_RWTexture_WriteOnly);
            initializePass->SetCallback(
                [rgRngTexture, &initRngPipeline, &device]
            {
                auto &commandBuffer = RGGetCommandBuffer();
                
                StaticShaderInfo<"InitRng">::Pass bindingGroupData;
                bindingGroupData.RngTexture = rgRngTexture;
                bindingGroupData.Resolution = rgRngTexture->GetSize();
                auto bindingGroup = device->CreateBindingGroupWithCachedLayout(bindingGroupData);

                commandBuffer.BindComputePipeline(initRngPipeline);
                commandBuffer.BindComputeGroup(0, bindingGroup);
                commandBuffer.DispatchWithThreadCount(rgRngTexture->GetWidth(), rgRngTexture->GetHeight());
            });
        }
        else
        {
            rgAccumulateTexture = graph->RegisterTexture(accumulateTexture);
            rgRngTexture = graph->RegisterTexture(rngTexture);
        }

        auto ptPass = graph->CreatePass("Trace");
        ptPass->Use(renderTarget,         RG::CS_RWTexture_WriteOnly);
        ptPass->Use(rgAccumulateTexture,  RG::CS_RWTexture);
        ptPass->Use(rgRngTexture,         RG::CS_RWTexture);
        ptPass->SetCallback([&]
        {
            StaticShaderInfo<"PathTracing">::Pass bindingGroupData;
            bindingGroupData.AccumulateTexture = rgAccumulateTexture;
            bindingGroupData.OutputTexture     = renderTarget;
            bindingGroupData.RngTexture        = rgRngTexture;
            bindingGroupData.Scene             = tlas;
            bindingGroupData.Geometries[0]     = room.primitives;
            bindingGroupData.Geometries[1]     = torus.primitives;
            bindingGroupData.Materials         = materialBuffer;
            bindingGroupData.EyePosition       = camera.GetPosition();
            bindingGroupData.FrustumA          = camera.GetWorldRays()[0];
            bindingGroupData.FrustumB          = camera.GetWorldRays()[1];
            bindingGroupData.FrustumC          = camera.GetWorldRays()[2];
            bindingGroupData.FrustumD          = camera.GetWorldRays()[3];
            bindingGroupData.Resolution        = rgAccumulateTexture->GetSize();
            auto bindingGroup = device->CreateBindingGroupWithCachedLayout(bindingGroupData);

            auto &commandBuffer = RGGetCommandBuffer();
            commandBuffer.BindComputePipeline(tracePipeline);
            commandBuffer.BindComputeGroup(0, bindingGroup);
            commandBuffer.DispatchWithThreadCount(renderTarget->GetWidth(), renderTarget->GetHeight());
        });

        RGBlitTexture(graph, "Blit", renderTarget, rgSwapchain, true);
        
        graph->SetCompleteFence(device->GetFrameFence());
        executer.Execute(graph);

        device->Present();
        device->EndFrame();
    }
}

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        Run();
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
