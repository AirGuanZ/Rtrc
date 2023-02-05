#include <Rtrc/Rtrc.h>

using namespace Rtrc;

void Run()
{
    Window window = WindowBuilder()
        .SetSize(800, 800)
        .SetTitle("Rtrc Sample: RayTracingTriangle")
        .Create();

    Box<Device> device = Device::CreateGraphicsDevice(
        window, RHI::Format::B8G8R8A8_UNorm, 3, RTRC_DEBUG, false, Device::EnableRayTracing);

    ResourceManager resourceManager(device.get());
    resourceManager.AddMaterialFiles($rtrc_get_files("Asset/Sample/05.RayTracingTriangle/*.*"));

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
}

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        Run();
    }
    catch(const std::exception &e)
    {
        LogError(e.what());
    }
}
