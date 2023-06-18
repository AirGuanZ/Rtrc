#include <Rtrc/Rtrc.h>

using namespace Rtrc;

constexpr int MAX_INSTANCE_COUNT = 128;

rtrc_group(TracePass)
{
    rtrc_define(RWTexture2D, AccumulateTexture);
    rtrc_define(RWTexture2D, OutputTexture);
    rtrc_define(RWTexture2D, RngTexture);

    rtrc_define  (RaytracingAccelerationStructure,      Scene);
    rtrc_define  (StructuredBuffer,                     Materials);
    rtrc_bindless(StructuredBuffer[MAX_INSTANCE_COUNT], Geometries);
    
    rtrc_uniform(float3, EyePosition);
    rtrc_uniform(float3, FrustumA);
    rtrc_uniform(float3, FrustumB);
    rtrc_uniform(float3, FrustumC);
    rtrc_uniform(float3, FrustumD);
    rtrc_uniform(uint2,  Resolution);
};

rtrc_group(InitRngPass)
{
    rtrc_define(RWTexture2D, RngTexture);
    rtrc_uniform(uint2, Resolution);
};

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
        .hostAccessType = RHI::BufferHostAccessType::Upload
    }, meshData.positionData.data());
    
    auto indexBuffer = device.CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size           = sizeof(uint32_t) * meshData.indexData.size(),
        .usage          = RHI::BufferUsage::AccelerationStructureBuildInput | RHI::BufferUsage::DeviceAddress,
        .hostAccessType = RHI::BufferHostAccessType::Upload
    }, meshData.indexData.data());

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

    return ret;
}

void Run()
{
    Window window = WindowBuilder()
        .SetSize(800, 800)
        .SetTitle("Rtrc Sample: PathTracing")
        .Create();

    Box<Device> device = Device::CreateGraphicsDevice(
        window, RHI::Format::B8G8R8A8_UNorm, 3, RTRC_DEBUG,
        false, Device::EnableRayTracing);

    ResourceManager resourceManager(device);
    resourceManager.AddMaterialFiles($rtrc_get_files("Asset/Sample/05.PathTracing/*.*"));

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
        .hostAccessType = RHI::BufferHostAccessType::Upload
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
            tlas, instanceArrayDesc, RHI::RayTracingAccelerationStructureBuildFlagBit::PreferFastTrace, nullptr);
    });

    // Pipeline

    auto traceShader = resourceManager.GetShader("PathTracing");
    auto tracePipeline = traceShader->GetComputePipeline();

    auto initRngShader = resourceManager.GetShader("InitRng");
    auto initRngPipeline = initRngShader->GetComputePipeline();

    // Render target

    RC<StatefulTexture> accumulateTexture;
    RC<StatefulTexture> rngTexture;

    // Camera

    Camera camera;
    camera.SetPosition({ 1.2f, 1.5f, -3 });
    camera.SetLookAt({ 0, 1, 0 }, { 0, 0, 0 });

    FreeCameraController cameraController;
    cameraController.SetCamera(camera);

    // Render loop

    RG::Executer executer(device);

    window.SetFocus();
    window.GetInput().LockCursor(true);
    RTRC_SCOPE_EXIT{ window.GetInput().LockCursor(false); };

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

        const float wOverH = static_cast<float>(window.GetFramebufferSize().x) / window.GetFramebufferSize().y;
        camera.SetProjection(Deg2Rad(60), wOverH, 0.1f, 1000.0f);
        bool needClear = false;
        if(window.GetInput().IsCursorLocked())
        {
            needClear = cameraController.UpdateCamera(window.GetInput(), timer);
        }
        camera.CalculateDerivedData();

        auto graph = device->CreateRenderGraph();
        auto rgSwapchain = graph->RegisterSwapchainTexture(device->GetSwapchain());
        auto renderTarget = graph->CreateTexture(RHI::TextureDesc
        {
            .dim                  = RHI::TextureDimension::Tex2D,
            .format               = RHI::Format::B8G8R8A8_UNorm,
            .width                = rgSwapchain->GetWidth(),
            .height               = rgSwapchain->GetHeight(),
            .arraySize            = 1,
            .mipLevels            = 1,
            .sampleCount          = 1,
            .usage                = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess,
            .initialLayout        = RHI::TextureLayout::Undefined,
            .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
        });

        if(accumulateTexture && (accumulateTexture->GetSize() != renderTarget->GetSize() || needClear))
        {
            accumulateTexture = nullptr;
        }

        RG::TextureResource *rgAccumulateTexture, *rgRngTexture;
        RG::Pass *initializePass = graph->CreatePass("Clear history texture");
        if(!accumulateTexture)
        {
            accumulateTexture = device->CreatePooledTexture(RHI::TextureDesc
            {
                .dim                  = RHI::TextureDimension::Tex2D,
                .format               = RHI::Format::R32G32B32A32_Float,
                .width                = renderTarget->GetWidth(),
                .height               = renderTarget->GetHeight(),
                .arraySize            = 1,
                .mipLevels            = 1,
                .sampleCount          = 1,
                .usage                = RHI::TextureUsage::ShaderResource |
                                        RHI::TextureUsage::UnorderAccess |
                                        RHI::TextureUsage::ClearColor,
                .initialLayout        = RHI::TextureLayout::Undefined,
                .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
            });
            accumulateTexture->SetName("AccumulateTexture");

            rngTexture = device->CreatePooledTexture(RHI::TextureDesc
            {
                .dim                  = RHI::TextureDimension::Tex2D,
                .format               = RHI::Format::R32_UInt,
                .width                = renderTarget->GetWidth(),
                .height               = renderTarget->GetHeight(),
                .arraySize            = 1,
                .mipLevels            = 1,
                .sampleCount          = 1,
                .usage                = RHI::TextureUsage::UnorderAccess,
                .initialLayout        = RHI::TextureLayout::Undefined,
                .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
            });
            rngTexture->SetName("RngTexture");

            rgAccumulateTexture = graph->RegisterTexture(accumulateTexture);
            rgRngTexture = graph->RegisterTexture(rngTexture);

            initializePass->Use(rgAccumulateTexture, RG::ClearDst);
            initializePass->Use(rgRngTexture, RG::CS_RWTexture_WriteOnly);
            initializePass->SetCallback(
                [rgAccumulateTexture, rgRngTexture, &initRngPipeline, &device]
                (RG::PassContext &context)
            {
                auto &commandBuffer = context.GetCommandBuffer();
                commandBuffer.ClearColorTexture2D(rgAccumulateTexture->Get(context), { 0, 0, 0, 0 });

                InitRngPass bindingGroupData;
                bindingGroupData.RngTexture = rgRngTexture->Get(context);
                bindingGroupData.Resolution = rgRngTexture->GetSize();
                auto bindingGroup = device->CreateBindingGroup(bindingGroupData);

                commandBuffer.BindComputePipeline(initRngPipeline);
                commandBuffer.BindComputeGroup(0, bindingGroup);
                commandBuffer.DispatchWithThreadCount(rgRngTexture->GetWidth(), rgRngTexture->GetHeight(), 1);
            });
        }
        else
        {
            rgAccumulateTexture = graph->RegisterTexture(accumulateTexture);
            rgRngTexture = graph->RegisterTexture(rngTexture);
        }

        auto ptPass = graph->CreatePass("Trace");
        ptPass->Use(renderTarget,         RG::CS_RWTexture_WriteOnly);
        ptPass->Use(rgAccumulateTexture, RG::CS_RWTexture);
        ptPass->Use(rgRngTexture,        RG::CS_RWTexture);
        ptPass->SetCallback([&](RG::PassContext &context)
        {
            TracePass bindingGroupData;
            bindingGroupData.AccumulateTexture = rgAccumulateTexture->Get(context);
            bindingGroupData.OutputTexture     = renderTarget->Get(context);
            bindingGroupData.RngTexture        = rgRngTexture->Get(context);
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
            auto bindingGroup = device->CreateBindingGroup(bindingGroupData);

            auto &commandBuffer = context.GetCommandBuffer();
            commandBuffer.BindComputePipeline(tracePipeline);
            commandBuffer.BindComputeGroup(0, bindingGroup);
            commandBuffer.DispatchWithThreadCount(renderTarget->GetWidth(), renderTarget->GetHeight(), 1);
        });

        auto copyPass = graph->CreateBlitTexture2DPass("Blit", renderTarget, rgSwapchain, true);
        copyPass->SetSignalFence(device->GetFrameFence());

        executer.Execute(graph);
        device->Present();
    }
}

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        Run();
    }
    catch(const Exception &e)
    {
        LogError("{}\n{}", e.what(), e.stacktrace());
    }
    catch(const std::exception &e)
    {
        LogError(e.what());
    }
}
