#include <Rtrc/ToolKit/Gizmo/GizmoRenderer.h>
#include <Rtrc/ToolKit/Gizmo/Gizmo.shader.outh>

#include "Rtrc/Graphics/RenderGraph/Graph.h"

RTRC_BEGIN

GizmoRenderer::GizmoRenderer(Ref<Device> device)
    : device_(device), pipelineCache_(device)
{

}

void GizmoRenderer::RenderImmediately(
    const GizmoBuilder &builder,
    CommandBuffer      &commandBuffer,
    const Matrix4x4f   &worldToClip,
    bool                reverseZ,
    float               rcpGamma)
{
    if(!builder.lines_.empty())
    {
        const auto vertices = ExtractVertices(builder.lines_);
        RenderImpl(vertices, commandBuffer, worldToClip, reverseZ, rcpGamma, true);
    }

    if(!builder.triangles_.empty())
    {
        const auto vertices = ExtractVertices(builder.triangles_);
        RenderImpl(vertices, commandBuffer, worldToClip, reverseZ, rcpGamma, false);
    }
}

void GizmoRenderer::AddRenderPass(
    const GizmoBuilder  &builder,
    Ref<RG::RenderGraph> renderGraph,
    RG::TextureResource *framebuffer,
    RG::TextureResource *depthBuffer,
    const Matrix4x4f    &worldToClip,
    bool                 reverseZ,
    float                rcpGamma)
{
    if(builder.lines_.empty() && builder.triangles_.empty())
    {
        return;
    }

    auto lineVertices = ExtractVertices(builder.lines_);
    auto triangleVertices = ExtractVertices(builder.triangles_);

    auto pass = renderGraph->CreatePass("RenderGizmo");
    pass->Use(framebuffer, RG::ColorAttachmentWriteOnly);
    if(depthBuffer)
        pass->Use(depthBuffer, RG::DepthStencilAttachment);
    pass->SetCallback([
        lv = std::move(lineVertices), tv = std::move(triangleVertices), this,
        framebuffer, depthBuffer, worldToClip, reverseZ, rcpGamma]
    {
        auto &cmds = RG::GetCurrentCommandBuffer();
        if(depthBuffer)
        {
            cmds.BeginRenderPass(
                ColorAttachment{ .renderTargetView = framebuffer->GetRtvImm() },
                DepthStencilAttachment{ .depthStencilView = depthBuffer->GetDsvImm() });
        }
        else
        {
            cmds.BeginRenderPass(ColorAttachment{ .renderTargetView = framebuffer->GetRtvImm() });
        }
        RTRC_SCOPE_EXIT{ cmds.EndRenderPass(); };

        cmds.SetViewports(framebuffer->GetViewport());
        cmds.SetScissors(framebuffer->GetScissor());

        if(!lv.empty())
        {
            this->RenderImpl(lv, cmds, worldToClip, reverseZ, rcpGamma, true);
        }

        if(!tv.empty())
        {
            this->RenderImpl(tv, cmds, worldToClip, reverseZ, rcpGamma, false);
        }
    });
}

std::vector<GizmoRenderer::Vertex> GizmoRenderer::ExtractVertices(Span<GizmoBuilder::Line> lines)
{
    std::vector<Vertex> vertices;
    vertices.reserve(lines.size() * 2);
    for(const auto &line : lines)
    {
               vertices.push_back({ line.a, line.color });
        vertices.push_back({ line.b, line.color });
    }
    return vertices;
}

std::vector<GizmoRenderer::Vertex> GizmoRenderer::ExtractVertices(Span<GizmoBuilder::Triangle> triangles)
{
    std::vector<Vertex> vertices;
    vertices.reserve(triangles.size() * 3);
    for(const auto &triangle : triangles)
    {
        vertices.push_back({ triangle.a, triangle.color });
        vertices.push_back({ triangle.b, triangle.color });
        vertices.push_back({ triangle.c, triangle.color });
    }
    return vertices;
}

void GizmoRenderer::RenderImpl(
    Span<Vertex>      vertices,
    CommandBuffer    &commandBuffer,
    const Matrix4x4f &worldToClip,
    bool              reverseZ,
    float             rcpGamma,
    bool              isLine)
{
    const auto colorFormats = commandBuffer.GetCurrentRenderPassColorFormats();
    const RHI::Format depthFormat = commandBuffer.GetCurrentRenderPassDepthStencilFormat();

    auto pipeline = pipelineCache_.GetGraphicsPipeline(GraphicsPipeline::Desc
    {
        .shader                 = device_->GetShader<"Rtrc/GizmoRenderer/RenderColoredPrimitives">(),
        .meshLayout             = RTRC_MESH_LAYOUT(Buffer(Attribute("POSITION", 0, Float3),
                                                          Attribute("COLOR", 0, Float3))),
        .primitiveTopology      = isLine ? RHI::PrimitiveTopology::LineList : RHI::PrimitiveTopology::TriangleList,
        .enableDepthTest        = depthFormat != RHI::Format::Unknown,
        .enableDepthWrite       = true,
        .depthCompareOp         = reverseZ ? RHI::CompareOp::GreaterEqual : RHI::CompareOp::LessEqual,
        .colorAttachmentFormats = { colorFormats.begin(), colorFormats.end() },
        .depthStencilFormat     = depthFormat
    });

    auto vertexBuffer = device_->CreateBuffer(RHI::BufferDesc
    {
        .size = sizeof(Vertex) * vertices.size(),
        .usage = RHI::BufferUsage::VertexBuffer,
        .hostAccessType = RHI::BufferHostAccessType::Upload
    }, "GizmoLineVertexBuffer");
    vertexBuffer->Upload(vertices.GetData(), 0, vertices.size() * sizeof(Vertex));

    RenderColoredPrimitives(pipeline, vertexBuffer, commandBuffer, worldToClip, rcpGamma);
}

void GizmoRenderer::RenderColoredPrimitives(
    const RC<GraphicsPipeline> &pipeline,
    const RC<Buffer>           &vertexBuffer,
    CommandBuffer              &commandBuffer,
    const Matrix4x4f           &worldToClip,
    float                       rcpGamma) const
{
    StaticShaderInfo<"Rtrc/GizmoRenderer/RenderColoredPrimitives">::Pass passData;
    passData.WorldToClip = worldToClip;
    passData.gamma = 1.0f / rcpGamma;
    auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

    commandBuffer.BindGraphicsPipeline(pipeline);
    commandBuffer.BindGraphicsGroup(0, passGroup);
    commandBuffer.SetVertexBuffer(0, vertexBuffer, sizeof(Vertex));
    commandBuffer.Draw(static_cast<int>(vertexBuffer->GetSize() / sizeof(Vertex)), 1, 0, 0);
}

RTRC_END