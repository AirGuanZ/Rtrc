//#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>
//
//RTRC_BEGIN
//
//RC<Mesh> GetFullscreenTriangleMesh(DynamicBufferManager &bufferManager, const RenderCamera &camera)
//{
//    auto layout = RTRC_MESH_LAYOUT(Buffer(
//        Attribute("POSITION", Float3),
//        Attribute("UV",       Float2),
//        Attribute("RAY",      Float3)));
//
//    MeshBuilder builder;
//    builder.SetLayout(layout);
//
//    struct Vertex
//    {
//        Vector3f position;
//        Vector2f uv;
//        Vector2f ray;
//    };
//
//    std::array<Vertex, 3> vertexData;
//    vertexData[0].position = Vector3f(); // TODO
//
//    auto vertexBuffer = bufferManager.Create();
//    vertexBuffer->SetData(vertexData.data(), sizeof(Vertex) * vertexData.size());
//
//    return {};
//}
//
//RTRC_END
