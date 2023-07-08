#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>

RTRC_BEGIN

namespace FullscreenPrimitiveDetail
{

    struct Vertex
    {
        Vector2f position;
        Vector2f uv;

        static const MeshLayout *GetLayout()
        {
            return RTRC_MESH_LAYOUT(Buffer(Attribute("POSITION", 0, Float2), Attribute("UV", 0, Float2)));
        }
    };

    struct VertexWithRay
    {
        Vector2f position;
        Vector2f uv;
        Vector3f worldRay;
        Vector3f cameraRay;

        static const MeshLayout *GetLayout()
        {
            return RTRC_MESH_LAYOUT(Buffer(
                Attribute("POSITION",   0, Float2),
                Attribute("UV",         0, Float2),
                Attribute("WORLD_RAY",  0, Float3),
                Attribute("CAMERA_RAY", 0, Float3)));
        }
    };

    template<typename V>
    void FillTrianglePositionAndUV(std::array<V, 3> &vertexData)
    {
        vertexData[0].position = Vector2f(-1, -3);
        vertexData[1].position = Vector2f(-1, +1);
        vertexData[2].position = Vector2f(+3, +1);
        vertexData[0].uv = Vector2f(0, 2);
        vertexData[1].uv = Vector2f(0, 0);
        vertexData[2].uv = Vector2f(2, 0);
    }

    template<typename V>
    void FillQuadPositionAndUV(std::array<V, 6> &vertexData)
    {
        vertexData[0].position = Vector2f(-1, -1); vertexData[0].uv = Vector2f(0, 1);
        vertexData[1].position = Vector2f(-1, +1); vertexData[1].uv = Vector2f(0, 0);
        vertexData[2].position = Vector2f(+1, +1); vertexData[2].uv = Vector2f(1, 0);
        vertexData[3].position = Vector2f(-1, -1); vertexData[3].uv = Vector2f(0, 1);
        vertexData[4].position = Vector2f(+1, +1); vertexData[4].uv = Vector2f(1, 0);
        vertexData[5].position = Vector2f(+1, -1); vertexData[5].uv = Vector2f(1, 1);
    }

} // namespace FullscreenPrimitiveDetail

const MeshLayout *GetFullscreenPrimitiveMeshLayout()
{
    return FullscreenPrimitiveDetail::Vertex::GetLayout();
}

const MeshLayout *GetFullscreenPrimitiveMeshLayoutWithWorldRay()
{
    return FullscreenPrimitiveDetail::VertexWithRay::GetLayout();
}

Mesh GetFullscreenTriangle(DynamicBufferManager &bufferManager)
{
    using namespace FullscreenPrimitiveDetail;

    std::array<Vertex, 3> vertexData;
    FillTrianglePositionAndUV(vertexData);

    auto vertexBuffer = bufferManager.Create();
    vertexBuffer->SetData(vertexData.data(), sizeof(Vertex) * vertexData.size(), false);

    MeshBuilder builder;
    builder.SetLayout(Vertex::GetLayout());
    builder.SetVertexBuffer(0, vertexBuffer);
    builder.SetVertexCount(3);
    return builder.CreateMesh();
}

Mesh GetFullscreenTriangle(DynamicBufferManager &bufferManager, const CameraRenderData &camera)
{
    using namespace FullscreenPrimitiveDetail;

    std::array<VertexWithRay, 3> vertexData;
    FillTrianglePositionAndUV(vertexData);

    {
        const Vector3f &c0 = camera.cameraRays[0];
        const Vector3f &c1 = camera.cameraRays[1];
        const Vector3f &c2 = camera.cameraRays[2];
        vertexData[0].cameraRay = 2.0f * c2 - c0;
        vertexData[1].cameraRay = c0;
        vertexData[2].cameraRay = 2.0f * c1 - c0;
    }

    {
        const Vector3f &c0 = camera.worldRays[0];
        const Vector3f &c1 = camera.worldRays[1];
        const Vector3f &c2 = camera.worldRays[2];
        vertexData[0].worldRay = 2.0f * c2 - c0;
        vertexData[1].worldRay = c0;
        vertexData[2].worldRay = 2.0f * c1 - c0;
    }
    
    auto vertexBuffer = bufferManager.Create();
    vertexBuffer->SetData(vertexData.data(), sizeof(VertexWithRay) * vertexData.size(), false);

    MeshBuilder builder;
    builder.SetLayout(VertexWithRay::GetLayout());
    builder.SetVertexBuffer(0, vertexBuffer);
    builder.SetVertexCount(3);
    return builder.CreateMesh();
}

Mesh GetFullscreenTriangle(DynamicBufferManager &bufferManager, const Camera &camera)
{
    return GetFullscreenTriangle(bufferManager, camera.GetRenderCamera());
}

Mesh GetFullscreenQuad(DynamicBufferManager &bufferManager)
{
    using namespace FullscreenPrimitiveDetail;

    std::array<Vertex, 6> vertexData;
    FillQuadPositionAndUV(vertexData);

    auto vertexBuffer = bufferManager.Create();
    vertexBuffer->SetData(vertexData.data(), sizeof(Vertex) *vertexData.size(), false);

    MeshBuilder builder;
    builder.SetLayout(Vertex::GetLayout());
    builder.SetVertexBuffer(0, vertexBuffer);
    builder.SetVertexCount(6);
    return builder.CreateMesh();
}

Mesh GetFullscreenQuad(DynamicBufferManager &bufferManager, const CameraRenderData &camera)
{
    using namespace FullscreenPrimitiveDetail;

    std::array<VertexWithRay, 6> vertexData;
    FillQuadPositionAndUV(vertexData);

    vertexData[0].cameraRay = camera.cameraRays[2];
    vertexData[1].cameraRay = camera.cameraRays[0];
    vertexData[2].cameraRay = camera.cameraRays[1];
    vertexData[3].cameraRay = camera.cameraRays[2];
    vertexData[4].cameraRay = camera.cameraRays[1];
    vertexData[5].cameraRay = camera.cameraRays[3];
    
    vertexData[0].worldRay = camera.worldRays[2];
    vertexData[1].worldRay = camera.worldRays[0];
    vertexData[2].worldRay = camera.worldRays[1];
    vertexData[3].worldRay = camera.worldRays[2];
    vertexData[4].worldRay = camera.worldRays[1];
    vertexData[5].worldRay = camera.worldRays[3];

    auto vertexBuffer = bufferManager.Create();
    vertexBuffer->SetData(vertexData.data(), sizeof(VertexWithRay) * vertexData.size(), false);

    MeshBuilder builder;
    builder.SetLayout(VertexWithRay::GetLayout());
    builder.SetVertexBuffer(0, vertexBuffer);
    builder.SetVertexCount(6);
    return builder.CreateMesh();
}

Mesh GetFullscreenQuad(DynamicBufferManager &bufferManager, const Camera &camera)
{
    return GetFullscreenQuad(bufferManager, camera.GetRenderCamera());
}

RTRC_END
