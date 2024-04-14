#include <Rtrc/ToolKit/OT2D/OT2D.h>

#include "GridAlignment.h"
#include "FADM.shader.outh"

namespace GridAlignment
{

    // ret[0] == 0
    // ret[n-1] == 1
    template<int Axis>
    std::vector<float> AccumulateVertexDensity_Deprecated(const InputMesh &inputMesh, int n)
    {
        // Accumulate density

        constexpr int delta = 5;
        std::vector<float> pdf(n - 1, 1);
        for(int vi : inputMesh.sharpFeatures.corners)
        {
            const float center = inputMesh.parameterSpacePositions[vi][Axis];
            const int ci = std::min(static_cast<int>(center * static_cast<float>(n - 1)), n - 1);
            for(int i = ci - delta; i <= ci + delta; ++i)
            {
                if(i >= 0 && i < n - 1)
                {
                    pdf[i] += 1.0f / static_cast<float>(std::abs(ci - i) + 1);
                }
            }
        }

        // Normalize

        float sum = 0;
        for(float &v : pdf)
        {
            v = std::min(v, 16.0f);
            sum += v;
        }
        const float rcpSum = 1 / std::max(sum, 0.001f);
        for(float& v : pdf)
        {
            v *= rcpSum;
        }

        // pdf -> cdf

        std::vector<float> ret(n);
        for(int i = 1; i < n; ++i)
        {
            ret[i] = ret[i - 1] + pdf[i - 1];
        }
        ret[n - 1] = 1;
        
        return ret;
    }

    Image<float> AccumulateVertexDensity2D(const InputMesh &inputMesh, int n)
    {
        Image<float> ret(n, n);
        for(auto &v : ret)
        {
            v = 0.5f;
        }

        constexpr int delta = 2;
        for(unsigned vi = 0; vi < inputMesh.positions.size(); ++vi)
        {
            const Vector2f &pos = inputMesh.parameterSpacePositions[vi];
            const float importance = inputMesh.sharpFeatures.vertexImportance[vi];
            const int cx = (std::min)(static_cast<int>(pos.x * (n - 1)), n - 1);
            const int cy = (std::min)(static_cast<int>(pos.y * (n - 1)), n - 1);
            for(int iy = cy - delta; iy <= cy + delta; ++iy)
            {
                if(iy < 0 || iy >= n)
                {
                    continue;
                }
                for(int ix = cx - delta; ix <= cx + delta; ++ix)
                {
                    if(ix < 0 || ix >= n)
                    {
                        continue;
                    }
                    ret(ix, iy) += 1.0f / (Length(Vector2i(ix - cx, iy - cy).To<float>()) + 1.0f) * importance;
                }
            }
        }

        return ret;
    }

    std::vector<float> SampleAdaptively_Deprecated(const std::vector<float> &cdf, int n)
    {
        std::vector<float> ret;
        ret.reserve(n);
        int s = 0;
        for(int i = 0; i < n; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(n - 1);
            while(t > cdf[s + 1])
            {
                ++s;
            }

            const float u0 = static_cast<float>(s + 0) / static_cast<float>(cdf.size() - 1);
            const float u1 = static_cast<float>(s + 1) / static_cast<float>(cdf.size() - 1);
            const float ut = (t - cdf[s]) / (cdf[s + 1] - cdf[s]);
            ret.push_back(std::lerp(u0, u1, ut));
        }
        return ret;
    }

    Image<GridPoint> CreateGrid(const Vector2u& res, const InputMesh &inputMesh, bool adaptive)
    {
        if(inputMesh.sharpFeatures.corners.empty())
        {
            adaptive = false;
        }

        std::vector<float> us, vs;
        if(adaptive)
        {
            auto cdfu = AccumulateVertexDensity_Deprecated<0>(inputMesh, res.x * 3);
            us = SampleAdaptively_Deprecated(cdfu, res.x);

            auto cdfv = AccumulateVertexDensity_Deprecated<1>(inputMesh, res.y * 3);
            vs = SampleAdaptively_Deprecated(cdfv, res.y);
        }
        else
        {
            us.resize(res.x);
            vs.resize(res.y);
            for(unsigned i = 0; i < res.x; ++i)
            {
                us[i] = static_cast<float>(i) / static_cast<float>(res.x - 1);
            }
            for(unsigned i = 0; i < res.y; ++i)
            {
                vs[i] = static_cast<float>(i) / static_cast<float>(res.y - 1);
            }
        }

        Image<GridPoint> ret(res.x, res.y);
        for(unsigned y = 0; y < res.y; ++y)
        {
            const float v = vs[y];
            for(unsigned x = 0; x < res.x; ++x)
            {
                const float u = us[x];
                auto &grid = ret(x, y);
                grid.uv = { u, v };
                grid.referenceUV = grid.uv;
            }
        }
        return ret;
    }

    Image<GridPoint> CreateGridEx(
        DeviceRef                  device,
        Ref<GraphicsPipelineCache> pipelineCache,
        const Vector2u            &res,
        const InputMesh           &inputMesh,
        bool                       adaptive)
    {
        if(!adaptive)
        {
            return CreateGrid(res, inputMesh, false);
        }

        const auto density = AccumulateVertexDensity2D(inputMesh, 2 * res.x).To<double>();
        auto transportMapData = OT2D().Solve(density).To<Vector2f>();
        auto transportMap = device->CreateAndUploadTexture(
        {
            .format = RHI::Format::R32G32_Float,
            .width = transportMapData.GetWidth(),
            .height = transportMapData.GetHeight(),
            .usage = RHI::TextureUsage::ShaderResource
        }, transportMapData.GetData(), RHI::TextureLayout::ShaderTexture);

        Image<Vector2f> inverseTransportMap;
        {
            RC<Buffer> vertexBuffer, indexBuffer;
            const Vector2u transportGridSize = transportMapData.GetSize() - Vector2u(1);
            {
                auto vertexData = GenerateUniformGridVertices(transportGridSize.x, transportGridSize.y);
                vertexBuffer = device->CreateAndUploadStructuredBuffer(
                    RHI::BufferUsage::VertexBuffer, Span<Vector2i>(vertexData));
            }
            {
                auto indexData = GenerateUniformGridTriangleIndices(transportGridSize.x, transportGridSize.y);
                indexBuffer = device->CreateAndUploadStructuredBuffer(
                    RHI::BufferUsage::IndexBuffer, Span<uint32_t>(indexData));
            }

            auto graph = device->CreateRenderGraph();

            auto gridMap = graph->CreateTexture(
            {
                .format = RHI::Format::R32G32B32A32_Float,
                .width = res.x,
                .height = res.y,
                .usage = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource | RHI::TextureUsage::TransferSrc,
                .clearValue = RHI::ColorClearValue{ 0, 0, 0, 0 }
            });
            {
                using Shader = RtrcShader::FADM::InverseTransportMap;
                RGAddRenderPass<true>(graph, Shader::Name, RGColorAttachment
                {
                    .rtv = gridMap->GetRtv(),
                    .loadOp = RHI::AttachmentLoadOp::Clear,
                    .clearValue = RHI::ColorClearValue{ 0, 0, 0, 0 }
                }, [&]
                {
                    Shader::Pass passData;
                    passData.TransportMap = transportMap;
                    passData.inputRes = transportGridSize.To<float>();
                    passData.res = res.To<float>();
                    auto passGroup = device->CreateBindingGroupWithCachedLayout(passData);

                    auto pipeline = pipelineCache->Get(
                    {
                        .shader = device->GetShader<Shader::Name>(),
                        .meshLayout = RTRC_STATIC_MESH_LAYOUT(Buffer(Attribute("COORD", Int2))),
                        .attachmentState = RTRC_ATTACHMENT_STATE
                        {
                            .colorAttachmentFormats = { gridMap->GetFormat() }
                        }
                    });

                    auto &commandBuffer = RGGetCommandBuffer();
                    commandBuffer.BindGraphicsPipeline(pipeline);
                    commandBuffer.BindGraphicsGroups(passGroup);
                    commandBuffer.SetVertexBuffer(0, vertexBuffer, sizeof(Vector2i));
                    commandBuffer.SetIndexBuffer(indexBuffer, RHI::IndexFormat::UInt32);
                    commandBuffer.DrawIndexed(indexBuffer->GetSize() / sizeof(uint32_t), 1, 0, 0, 0);
                });
            }

            Image<Vector4f> gridData(gridMap->GetWidth(), gridMap->GetHeight());
            RGReadbackTexture(graph, "Readback grid data", gridMap, 0, 0, gridData.GetData());

            RGExecuter(device).Execute(graph, false);

            inverseTransportMap = gridData.To<Vector2f>();
        }
        
        auto FixBoundary = [&](const Vector2i &coord)
        {
            auto &term = inverseTransportMap(coord.x, coord.y);
            if(coord.x == 0)
            {
                term.x = 0;
            }
            else if(coord.x == inverseTransportMap.GetSWidth() - 1)
            {
                term.x = 1;
            }
            if(coord.y == 0)
            {
                term.y = 0;
            }
            else if(coord.y == inverseTransportMap.GetSHeight() - 1)
            {
                term.y = 1;
            }
        };
        for(int x = 0; x < inverseTransportMap.GetSWidth(); ++x)
        {
            FixBoundary({ x, 0,                                  });
            FixBoundary({ x, inverseTransportMap.GetSHeight() - 1});
        }
        for(int y = 1; y + 1 < inverseTransportMap.GetSHeight() - 1; ++y)
        {
            FixBoundary({ 0,                                   y });
            FixBoundary({ inverseTransportMap.GetSWidth() - 1, y });
        }

        Image<GridPoint> ret(res.x, res.y);
        for(unsigned y = 0; y < res.y; ++y)
        {
            for(unsigned x = 0; x < res.x; ++x)
            {
                auto &grid = ret(x, y);
                grid.uv = inverseTransportMap(x, y);
                grid.referenceUV = grid.uv;
            }
        }
        return ret;
    }

    void AlignCorners(Image<GridPoint>& grid, const InputMesh &inputMesh)
    {
        RTRC_LOG_INFO_SCOPE("Align grid to sharp corners");

        const Vector2i res = grid.GetSSize();

        std::set<int> corners = { inputMesh.sharpFeatures.corners.begin(), inputMesh.sharpFeatures.corners.end() };
        while(!corners.empty())
        {
            const int vi = *corners.begin();
            corners.erase(corners.begin());
            
            const Vector2f &pos = inputMesh.parameterSpacePositions[vi];

            Vector2i coord;
            float minDist = std::numeric_limits<float>::max();
            for(int y = 1; y < res.y - 1; ++y)
            {
                for(int x = 1; x < res.x - 1; ++x)
                {
                    auto &g = grid(x, y);
                    const float dist = Length(g.referenceUV - pos);
                    if(dist < minDist)
                    {
                        minDist = dist;
                        coord = { x, y };
                    }
                }
            }

            auto &g = grid(coord.x, coord.y);
            if(g.corner >= 0)
            {
                const Vector2f &gpos = inputMesh.parameterSpacePositions[g.corner];
                if(Length(g.referenceUV - pos) > Length(g.referenceUV - gpos))
                {
                    continue;
                }
            }

            g.corner = vi;
            g.uv = pos;
        }
    }

    void AlignSegments(Image<GridPoint> &grid, const InputMesh &inputMesh)
    {
        RTRC_LOG_INFO_SCOPE("Align grid to sharp edges");

        for(const SharpSegment &segment : inputMesh.sharpFeatures.segments)
        {
            for(const int sei : segment.edges)
            {
                const SharpEdge &sharpEdge = inputMesh.sharpFeatures.edges[sei];

                const Vector2f p0 = inputMesh.parameterSpacePositions[sharpEdge.v0];
                const Vector2f p1 = inputMesh.parameterSpacePositions[sharpEdge.v1];

                const Vector2f lower = Min(p0, p1);
                const Vector2f upper = Max(p0, p1);

                int ix0 = 0;
                while(ix0 + 1 < grid.GetSWidth() && grid(ix0 + 1, 0).referenceUV.x < lower.x)
                {
                    ++ix0;
                }
                ix0 = std::max(0, ix0 - 2);

                int iy0 = 0;
                while(iy0 + 1 < grid.GetSHeight() && grid(0, iy0 + 1).referenceUV.y < lower.y)
                {
                    ++iy0;
                }
                iy0 = std::max(0, iy0 - 2);

                int ix1 = ix0;
                while(ix1 < grid.GetSWidth() && grid(ix1, 0).referenceUV.x < upper.x)
                {
                    ++ix1;
                }
                ix1 = std::min(ix1 + 2, grid.GetSWidth() - 1);

                int iy1 = iy0;
                while(iy1 < grid.GetSHeight() && grid(0, iy1).referenceUV.y < upper.y)
                {
                    ++iy1;
                }
                iy1 = std::min(iy1 + 2, grid.GetSHeight() - 1);

                auto HandleGridEdge = [&](const Vector2i &ga, const Vector2i &gb)
                {
                    const Vector2f ta = grid(ga.x, ga.y).uv;
                    const Vector2f tb = grid(gb.x, gb.y).uv;
                    const Vector2f a = tb - ta;
                    const Vector2f b = p0 - p1;
                    const Vector2f c = p0 - ta;
                    const float D = a.x * b.y - a.y * b.x;
                    if(std::abs(D) < 1e-6f)
                    {
                        return;
                    }
                    const float m = (c.x * b.y - c.y * b.x) / D;
                    const float n = (a.x * c.y - a.y * c.x) / D;
                    if(m < 0 || m > 1 || n < 0 || n > 1)
                    {
                        return;
                    }

                    auto IsOnBoundary = [&](const Vector2i &coord)
                    {
                        return coord.x == 0 || coord.x == grid.GetSWidthMinusOne() ||
                               coord.y == 0 || coord.y == grid.GetSHeightMinusOne();
                    };

                    Vector2i coord;
                    if(IsOnBoundary(ga))
                    {
                        coord = gb;
                    }
                    else if(IsOnBoundary(gb))
                    {
                        coord = ga;
                    }
                    else
                    {
                        coord = m <= 0.5f ? ga : gb;
                    }
                    if(IsOnBoundary(coord))
                    {
                        return;
                    }

                    auto &g = grid(coord.x, coord.y);
                    if(g.corner < 0 && g.edge < 0)
                    {
                        g.edge = sei;
                        g.uv = ta + m * (tb - ta);
                    }
                };

                for(int iy = iy0; iy <= iy1; ++iy)
                {
                    for(int ix = ix0; ix < ix1; ++ix)
                    {
                        HandleGridEdge({ ix, iy }, { ix + 1, iy });
                    }
                }
                for(int ix = ix0; ix <= ix1; ++ix)
                {
                    for(int iy = iy0; iy < iy1; ++iy)
                    {
                        HandleGridEdge({ ix, iy }, { ix, iy + 1 });
                    }
                }
            }
        }
    }

    RC<Texture> UploadGrid(DeviceRef device, const Image<GridPoint>& grid)
    {
        Image<Vector2f> uvData(grid.GetWidth(), grid.GetHeight());
        for(unsigned y = 0; y < grid.GetHeight(); ++y)
        {
            for(unsigned x = 0; x < grid.GetWidth(); ++x)
            {
                uvData(x, y) = grid(x, y).uv;
            }
        }
        return device->CreateAndUploadTexture(RHI::TextureDesc
        {
            .format = RHI::Format::R32G32_Float,
            .width = uvData.GetWidth(),
            .height = uvData.GetHeight(),
            .usage = RHI::TextureUsage::ShaderResource
        }, uvData.GetData(), RHI::TextureLayout::ShaderTexture);
    }

    RC<Buffer> GenerateIndexBufferForGrid(DeviceRef device, const InputMesh &inputMesh, const Image<GridPoint>& grid)
    {
        auto &edges = inputMesh.sharpFeatures.edges;
        const int w = grid.GetSWidth(), h = grid.GetSHeight();
        Image<uint32_t> direction(w - 1, h - 1);
        for(int y = 0; y < h - 1; ++y)
        {
            for(int x = 0; x < w - 1; ++x)
            {
                auto IsCornerAndEdgeConnected = [&](int corner, int edge)
                {
                    if(corner < 0 || edge < 0)
                    {
                        return false;
                    }
                    return edges[edge].v0 == corner || edges[edge].v1 == corner;
                };

                const GridPoint &x0y1 = grid(x + 0, y + 1);
                const GridPoint &x1y0 = grid(x + 1, y + 0);
                if((x1y0.corner >= 0 && x0y1.corner >= 0) ||
                   IsCornerAndEdgeConnected(x0y1.corner, x1y0.edge) ||
                   IsCornerAndEdgeConnected(x1y0.corner, x0y1.edge) ||
                   (x1y0.edge >= 0 && x1y0.edge == x0y1.edge))
                {
                    direction(x, y) = 1;
                }
            }
        }

        std::vector<uint32_t> indices;
        indices.reserve((w - 1) * (h - 1) * 6);
        for(int y = 0; y < h - 1; ++y)
        {
            for(int x = 0; x < w - 1; ++x)
            {
                if(direction(x, y))
                {
                    indices.push_back(w * (y + 0) + (x + 0));
                    indices.push_back(w * (y + 1) + (x + 0));
                    indices.push_back(w * (y + 0) + (x + 1));

                    indices.push_back(w * (y + 0) + (x + 1));
                    indices.push_back(w * (y + 1) + (x + 0));
                    indices.push_back(w * (y + 1) + (x + 1));
                }
                else
                {
                    indices.push_back(w * (y + 0) + (x + 0));
                    indices.push_back(w * (y + 1) + (x + 0));
                    indices.push_back(w * (y + 1) + (x + 1));

                    indices.push_back(w * (y + 0) + (x + 0));
                    indices.push_back(w * (y + 1) + (x + 1));
                    indices.push_back(w * (y + 0) + (x + 1));
                }
            }
        }

        return device->CreateAndUploadStructuredBuffer(RHI::BufferUsage::IndexBuffer, Span<uint32_t>(indices));
    }

} // namespace GridAlignment
