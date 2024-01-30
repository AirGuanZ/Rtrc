#include <Rtrc/Rtrc.h>

using namespace Rtrc;

#include "RayTracingTriangle.shader.outh"

void Run()
{
    Window window = WindowBuilder()
        .SetSize(800, 800)
        .SetTitle("Rtrc Sample: RayTracingTriangle")
        .Create();

    Box<Device> device = Device::CreateGraphicsDevice(
        window, RHI::BackendType::Default, RHI::Format::B8G8R8A8_UNorm, 3,
        RTRC_DEBUG, false, Device::EnableRayTracing);

    ResourceManager resourceManager(device);
    
    // Blas

    std::vector<Vector3f> triangleData =
    {
        { -1.0f, -1.0f, 0.0f },
        { +0.0f, +1.0f, 0.0f },
        { +1.0f, -1.0f, 0.0f }
    };

    auto triangleDataBuffer = device->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size           = sizeof(Vector3f) * triangleData.size(),
        .usage          = RHI::BufferUsage::AccelerationStructureBuildInput | RHI::BufferUsage::DeviceAddress,
        .hostAccessType = RHI::BufferHostAccessType::None
    }, triangleData.data());

    auto blas = device->CreateBlas();

    device->ExecuteAndWait([&](CommandBuffer &commandBuffer)
    {
        const RHI::RayTracingGeometryDesc geometry =
        {
            .type           = RHI::RayTracingGeometryType::Triangles,
            .primitiveCount = 1,
            .trianglesData  = RHI::RayTracingTrianglesGeometryData
            {
                .vertexFormat = RHI::RayTracingVertexFormat::Float3,
                .vertexStride = sizeof(Vector3f),
                .vertexCount  = 3,
                .indexFormat  = RHI::RayTracingIndexFormat::None,
                .hasTransform = false,
                .vertexData   = triangleDataBuffer->GetDeviceAddress()
            }
        };
        commandBuffer.BuildBlas(
            blas, geometry, RHI::RayTracingAccelerationStructureBuildFlagBit::PreferFastTrace, nullptr);
    });

    // Tlas

    const RHI::RayTracingInstanceData instanceData =
    {
        .transform3x4 =
        {
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 1, 0 }
        },
        .instanceCustomIndex          = 0,
        .instanceMask                 = 0xff,
        .instanceSbtOffset            = 0,
        .flags                        = 0,
        .accelerationStructureAddress = blas->GetRHIObject()->GetDeviceAddress().address
    };
    auto instanceDataBuffer = device->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size           = sizeof(instanceData),
        .usage          = RHI::BufferUsage::AccelerationStructureBuildInput | RHI::BufferUsage::DeviceAddress,
        .hostAccessType = RHI::BufferHostAccessType::None
    }, &instanceData);

    auto tlas = device->CreateTlas();

    device->ExecuteAndWait([&](CommandBuffer &commandBuffer)
    {
        const RHI::RayTracingInstanceArrayDesc instanceArrayDesc =
        {
            .instanceCount = 1,
            .instanceData  = instanceDataBuffer->GetDeviceAddress()
        };
        commandBuffer.BuildTlas(
            tlas, instanceArrayDesc, RHI::RayTracingAccelerationStructureBuildFlagBit::PreferFastTrace, nullptr);
    });

    // Pipeline

    auto shader = device->GetShader("RayTracingTriangle", true);

    assert(shader->GetRayGenShaderGroups().size() == 1);
    assert(shader->GetMissShaderGroups().size() == 1);
    assert(shader->GetHitShaderGroups().size() == 1);

    std::vector<RHI::RayTracingShaderGroup> shaderGroups;
    std::ranges::copy(shader->GetRayGenShaderGroups(), std::back_inserter(shaderGroups));
    std::ranges::copy(shader->GetMissShaderGroups(), std::back_inserter(shaderGroups));
    std::ranges::copy(shader->GetHitShaderGroups(), std::back_inserter(shaderGroups));

    auto bindingLayout = shader->GetBindingLayout();

    auto pipeline = device->CreateRayTracingPipeline(RayTracingPipeline::Desc
    {
        .shaders                = { shader },
        .shaderGroups           = shaderGroups,
        .bindingLayout          = std::move(bindingLayout),
        .maxRayPayloadSize      = 3 * sizeof(float),
        .maxRayHitAttributeSize = 2 * sizeof(float),
        .maxRecursiveDepth      = 1
    });

    // Shader binding table

    ShaderBindingTable sbt;
    {
        ShaderBindingTableBuilder sbtBuilder(device.get());
        auto startHandle = ShaderGroupHandle::FromPipeline(pipeline);

        auto raygenTable = sbtBuilder.AddSubtable();
        raygenTable->Resize(1);
        raygenTable->SetEntry(0, startHandle.Offset(0), nullptr, 0);

        auto missTable = sbtBuilder.AddSubtable();
        missTable->Resize(1);
        missTable->SetEntry(0, startHandle.Offset(1), nullptr, 0);

        auto hitTable = sbtBuilder.AddSubtable();
        hitTable->Resize(1);
        hitTable->SetEntry(0, startHandle.Offset(2), nullptr, 0);

        sbt = sbtBuilder.CreateShaderBindingTable();
    }

    // Render loop

    RGExecuter executer(device);

    window.SetFocus();
    device->BeginRenderLoop();
    RTRC_SCOPE_EXIT{ device->EndRenderLoop(); };

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

        auto graph = device->CreateRenderGraph();
        auto swapchainImage = graph->RegisterSwapchainTexture(device->GetSwapchain());
        auto renderTarget = graph->CreateTexture(RHI::TextureDesc
        {
            .dim                  = RHI::TextureDimension::Tex2D,
            .format               = RHI::Format::B8G8R8A8_UNorm,
            .width                = device->GetSwapchain()->GetRenderTargetDesc().width,
            .height               = device->GetSwapchain()->GetRenderTargetDesc().height,
            .arraySize            = 1,
            .mipLevels            = 1,
            .sampleCount          = 1,
            .usage                = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess,
            .initialLayout        = RHI::TextureLayout::Undefined,
            .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
        }, "RenderTarget");

        auto trianglePass = graph->CreatePass("DrawTriangle");
        trianglePass->Use(renderTarget, RG::CS_RWTexture_WriteOnly);
        trianglePass->SetCallback([&]
        {
            auto &commandBuffer = RGGetCommandBuffer();

            commandBuffer.BindRayTracingPipeline(pipeline);

            StaticShaderInfo<"RayTracingTriangle">::Pass bindingGroupData;
            bindingGroupData.OutputTextureRW = renderTarget;
            bindingGroupData.Scene = tlas;
            auto bindingGroup = device->CreateBindingGroup(bindingGroupData);
            commandBuffer.BindRayTracingGroup(0, bindingGroup);

            commandBuffer.Trace(
                renderTarget->GetWidth(), renderTarget->GetHeight(), 1u,
                sbt.GetSubtable(0),
                sbt.GetSubtable(1),
                sbt.GetSubtable(2),
                {});
        });

        BlitTexture(graph, "BlitToSwapchainImage", renderTarget, swapchainImage, true);

        graph->SetCompleteFence(device->GetFrameFence());
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
