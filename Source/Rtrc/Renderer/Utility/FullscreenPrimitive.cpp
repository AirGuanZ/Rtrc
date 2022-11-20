#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>

RTRC_BEGIN

namespace
{

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

} // namespace anonymous

RC<Mesh> GetFullscreenTriangle(DynamicBufferManager &bufferManager)
{
    struct Vertex
    {
        Vector2f position;
        Vector2f uv;
    };
    std::array<Vertex, 3> vertexData;
    FillTrianglePositionAndUV(vertexData);

    auto vertexBuffer = bufferManager.Create();
    vertexBuffer->SetData(vertexData.data(), sizeof(Vertex) * vertexData.size(), false);

    MeshBuilder builder;
    builder.SetLayout(RTRC_MESH_LAYOUT(Buffer(Attribute("POSITION", Float2), Attribute("UV", Float2))));
    builder.SetVertexBuffer(0, vertexBuffer);
    return ToRC(builder.CreateMesh());
}

RC<Mesh> GetFullscreenTriangle(DynamicBufferManager &bufferManager, const RenderCamera &camera)
{
    struct Vertex
    {
        Vector2f position;
        Vector2f uv;
        Vector3f worldRay;
        Vector3f cameraRay;
    };
    std::array<Vertex, 3> vertexData;
    FillTrianglePositionAndUV(vertexData);
    for(int i = 0; i < 4; ++i)
    {
        vertexData[i].cameraRay = camera.GetConstantBufferData().cameraRays[i];
        vertexData[i].worldRay = camera.GetConstantBufferData().worldRays[i];
    }
    
    auto vertexBuffer = bufferManager.Create();
    vertexBuffer->SetData(vertexData.data(), sizeof(Vertex) * vertexData.size(), false);

    MeshBuilder builder;
    builder.SetLayout(RTRC_MESH_LAYOUT(Buffer(
        Attribute("POSITION",   Float2),
        Attribute("UV",         Float2),
        Attribute("WORLD_RAY",  Float3),
        Attribute("CAMERA_RAY", Float3))));
    builder.SetVertexBuffer(0, vertexBuffer);
    return ToRC(builder.CreateMesh());
}

RTRC_END
