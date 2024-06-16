#include <Rtrc/Rtrc.h>

#include "DrawMesh.shader.outh"

using namespace Rtrc;

class HalfedgeMeshDemo : public SimpleApplication
{
    struct Vertex
    {
        Vector2f position;
        Vector3f color;

        bool operator<(const Vertex &rhs) const
        {
            return std::make_tuple(position, color) < std::make_tuple(rhs.position, rhs.color);
        }
    };

    void InitializeSimpleApplication(GraphRef graph) override
    {
        positions_ =
        {
            { -1, -1 },
            { -1, +1 },
            { +1, +1 },
            { +1, -1 }
        };
        std::vector<uint32_t> indices =
        {
            0, 1, 2,
            0, 2, 3
        };
        halfedgeMesh_ = FlatHalfedgeMesh::Build(indices);

        SplitEdge(2);
        SplitEdge(halfedgeMesh_.Succ(halfedgeMesh_.Succ(halfedgeMesh_.Twin(halfedgeMesh_.Succ(2)))));
        SplitEdge(halfedgeMesh_.Succ(2));
        SplitFace(1);
        SplitFace(6);

        GenerateVisualizationData();
    }

    void SplitEdge(int h)
    {
        auto &m = halfedgeMesh_;

        const int a0 = h;
        const int a1 = m.Succ(a0);
        const int v0 = m.Vert(a0);
        const int v1 = m.Vert(a1);

        m.SplitEdge(a0);

        positions_.push_back(0.5f * (positions_[v0] + positions_[v1]));
    }

    void SplitFace(int f)
    {
        auto &m = halfedgeMesh_;

        const int v0 = m.Vert(3 * f + 0);
        const int v1 = m.Vert(3 * f + 1);
        const int v2 = m.Vert(3 * f + 2);

        m.SplitFace(f);

        positions_.push_back(1.0f / 3 * (positions_[v0] + positions_[v1] + positions_[v2]));
    }

    void UpdateSimpleApplication(GraphRef graph) override
    {
        if(input_->IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        auto framebuffer = graph->RegisterSwapchainTexture(GetSwapchain());
        Render(graph, framebuffer);
    }

    void GenerateVisualizationData()
    {
        std::vector<Vertex>   vertices;
        std::vector<uint32_t> lineIndices;
        std::vector<uint32_t> triangleIndices;

        std::map<Vertex, uint32_t> vertexMap;
        auto NewVertex = [&](const Vertex &v)
        {
            auto it = vertexMap.find(v);
            if(it == vertexMap.end())
            {
                const uint32_t index = vertices.size();
                vertices.push_back(v);
                it = vertexMap.emplace(v, index).first;
            }
            return it->second;
        };

        constexpr Vector3f FACE_COLOR = { 0.05f, 0.05f, 0.05f };
        constexpr Vector3f WIRE_COLOR = { 0.8f, 0.8f, 0.8f };
        constexpr Vector3f ARROW_COLOR = { 0.7f, 0.2f, 0.2f };

        auto AddArrow = [&](const Vector2f &start, const Vector2f &end)
        {
            lineIndices.push_back(NewVertex({ start, ARROW_COLOR }));
            lineIndices.push_back(NewVertex({ end, ARROW_COLOR }));

            const Vector2f e2s = 0.02f * Normalize(start - end);
            const Vector2f e21 = end + (Matrix3x3f::RotateZ(Deg2Rad(+45)) * Vector3f(e2s, 0)).xy();
            lineIndices.push_back(NewVertex({ end, ARROW_COLOR }));
            lineIndices.push_back(NewVertex({ e21, ARROW_COLOR }));
        };

        auto AddHalfedge = [&](const Vector2f &a, const Vector2f &b, const Vector2f &c)
        {
            const Vector2f ab = b - a;
            const Vector2f ac = c - a;
            Vector2f offsetDirection = Normalize(Vector2f(ab.y, -ab.x));
            if(Dot(offsetDirection, ac) < 0)
            {
                offsetDirection = -offsetDirection;
            }

            const Vector2f center = 0.5f * (a + b);
            const float length = 0.3f * Length(ab);
            const Vector2f start = center - 0.5f * length * Normalize(ab) + 0.01f * offsetDirection;
            const Vector2f end   = center + 0.5f * length * Normalize(ab) + 0.01f * offsetDirection;
            AddArrow(start, end);
        };

        for(int f = 0; f < halfedgeMesh_.F(); ++f)
        {
            const int h0 = 3 * f + 0;
            const int h1 = 3 * f + 1;
            const int h2 = 3 * f + 2;

            const int v0 = halfedgeMesh_.Vert(h0);
            const int v1 = halfedgeMesh_.Vert(h1);
            const int v2 = halfedgeMesh_.Vert(h2);

            const Vector2f &p0 = positions_[v0];
            const Vector2f &p1 = positions_[v1];
            const Vector2f &p2 = positions_[v2];

            triangleIndices.push_back(NewVertex({ p0, FACE_COLOR }));
            triangleIndices.push_back(NewVertex({ p1, FACE_COLOR }));
            triangleIndices.push_back(NewVertex({ p2, FACE_COLOR }));

            if(h0 > halfedgeMesh_.Twin(h0))
            {
                lineIndices.push_back(NewVertex({ p0, WIRE_COLOR }));
                lineIndices.push_back(NewVertex({ p1, WIRE_COLOR }));
            }
            if(h1 > halfedgeMesh_.Twin(h1))
            {
                lineIndices.push_back(NewVertex({ p1, WIRE_COLOR }));
                lineIndices.push_back(NewVertex({ p2, WIRE_COLOR }));
            }
            if(h2 > halfedgeMesh_.Twin(h2))
            {
                lineIndices.push_back(NewVertex({ p2, WIRE_COLOR }));
                lineIndices.push_back(NewVertex({ p0, WIRE_COLOR }));
            }

            AddHalfedge(p0, p1, p2);
            AddHalfedge(p1, p2, p0);
            AddHalfedge(p2, p0, p1);
        }

        vertexBuffer_ = device_->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size = sizeof(Vertex) * vertices.size(),
            .usage = RHI::BufferUsage::VertexBuffer
        }, vertices.data());

        lineIndexBuffer_ = device_->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size = sizeof(uint32_t) * lineIndices.size(),
            .usage = RHI::BufferUsage::IndexBuffer
        }, lineIndices.data());
        
        triangleIndexBuffer_ = device_->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size = sizeof(uint32_t) * triangleIndices.size(),
            .usage = RHI::BufferUsage::IndexBuffer
        }, triangleIndices.data());
    }

    void Render(GraphRef graph, RGTexture renderTarget)
    {
        if(!linePipeline_)
        {
            trianglePipeline_ = device_->CreateGraphicsPipeline(GraphicsPipelineDesc
            {
                .shader = device_->GetShader<RtrcShader::Draw::Name>(),
                .meshLayout = RTRC_STATIC_MESH_LAYOUT(Buffer(
                    Attribute("POSITION", Float2), Attribute("COLOR", Float3))),
                .attachmentState = RTRC_ATTACHMENT_STATE
                {
                    .colorAttachmentFormats = { renderTarget->GetFormat() }
                }
            });
            linePipeline_ = device_->CreateGraphicsPipeline(GraphicsPipelineDesc
            {
                .shader = device_->GetShader<RtrcShader::Draw::Name>(),
                .meshLayout = RTRC_STATIC_MESH_LAYOUT(Buffer(
                    Attribute("POSITION", Float2), Attribute("COLOR", Float3))),
                .rasterizerState = RTRC_STATIC_RASTERIZER_STATE(
                {
                    .primitiveTopology = RHI::PrimitiveTopology::LineList
                }),
                .attachmentState = RTRC_ATTACHMENT_STATE
                {
                    .colorAttachmentFormats = { renderTarget->GetFormat() }
                }
            });
        }

        RGAddRenderPass<true>(
            graph, "Draw", RGColorAttachment
            {
                .rtv = renderTarget->GetRtv(),
                .loadOp = RHI::AttachmentLoadOp::Clear,
                .clearValue = { 0, 0, 0, 0 }
            },
            [this]
            {
                const float wOverH = window_->GetFramebufferWOverH();
                Vector2f range;
                if(wOverH > 1)
                {
                    range.y = 0.9f;
                    range.x = range.y / wOverH;
                }
                else
                {
                    range.x = 0.9f;
                    range.y = range.x * wOverH;
                }

                RtrcShader::Draw::Pass passData;
                passData.lower = -range;
                passData.upper = range;
                auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

                auto &commandBuffer = RGGetCommandBuffer();

                commandBuffer.BindGraphicsPipeline(trianglePipeline_);
                commandBuffer.BindGraphicsGroups(passGroup);
                commandBuffer.SetVertexBuffer(0, vertexBuffer_, sizeof(Vertex));
                commandBuffer.SetIndexBuffer(triangleIndexBuffer_, RHI::IndexFormat::UInt32);
                commandBuffer.DrawIndexed(triangleIndexBuffer_->GetSize() / sizeof(uint32_t), 1, 0, 0, 0);

                commandBuffer.BindGraphicsPipeline(linePipeline_);
                commandBuffer.BindGraphicsGroups(passGroup);
                commandBuffer.SetVertexBuffer(0, vertexBuffer_, sizeof(Vertex));
                commandBuffer.SetIndexBuffer(lineIndexBuffer_, RHI::IndexFormat::UInt32);
                commandBuffer.DrawIndexed(lineIndexBuffer_->GetSize() / sizeof(uint32_t), 1, 0, 0, 0);
            });
    }

    std::vector<Vector2f> positions_;
    FlatHalfedgeMesh      halfedgeMesh_;

    RC<Buffer> vertexBuffer_;
    RC<Buffer> triangleIndexBuffer_;
    RC<Buffer> lineIndexBuffer_;

    RC<GraphicsPipeline> trianglePipeline_;
    RC<GraphicsPipeline> linePipeline_;
};

RTRC_APPLICATION_MAIN(
    HalfedgeMeshDemo,
    .title             = "Rtrc Sample: Halfedge mesh",
    .width             = 640,
    .height            = 480,
    .maximized         = true,
    .vsync             = true,
    .debug             = RTRC_DEBUG,
    .rayTracing        = false,
    .backendType       = RHI::BackendType::Default,
    .enableGPUCapturer = false)
