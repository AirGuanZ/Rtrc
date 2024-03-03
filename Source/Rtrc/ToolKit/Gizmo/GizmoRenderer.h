#pragma once

#include <Rtrc/Graphics/Device/PipelineCache.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/ToolKit/Gizmo/GizmoBuilder.h>

RTRC_BEGIN

class GizmoRenderer : public Uncopyable
{
public:

    explicit GizmoRenderer(Ref<Device> device);

    void RenderImmediately(
        const GizmoBuilder &builder,
        CommandBuffer      &commandBuffer,
        const Matrix4x4f   &worldToClip,
        bool                reverseZ = false,
        float               rcpGamma = 1 /* 2.2 */);

    void AddRenderPass(
        const GizmoBuilder  &builder,
        GraphRef             renderGraph,
        RGTexture            framebuffer,
        RGTexture            depthBuffer, // depth buffer is optional
        const Matrix4x4f&    worldToClip,
        bool                 reverseZ = false,
        float                rcpGamma = 1 /* 2.2 */);

private:

    struct Vertex
    {
        Vector3f position;
        Vector3f color;
    };

    static std::vector<Vertex> ExtractVertices(Span<GizmoBuilder::Line> lines);
    static std::vector<Vertex> ExtractVertices(Span<GizmoBuilder::Triangle> triangles);

    void RenderImpl(
        Span<Vertex>      vertices,
        CommandBuffer    &commandBuffer,
        const Matrix4x4f &worldToClip,
        bool              reverseZ,
        float             rcpGamma,
        bool              isLine);

    void RenderColoredPrimitives(
        const RC<GraphicsPipeline> &pipeline,
        const RC<Buffer>           &vertexBuffer,
        CommandBuffer              &commandBuffer,
        const Matrix4x4f           &worldToClip,
        float                       rcpGamma) const;

    Ref<Device>              device_;
    GraphicsPipelineLRUCache pipelineCache_;
};

RTRC_END
