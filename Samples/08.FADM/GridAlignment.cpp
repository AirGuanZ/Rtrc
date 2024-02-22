#include "GridAlignment.h"

namespace GridAlignment
{

    // ret[0] == 0
    // ret[n-1] == 1
    template<int Axis>
    std::vector<float> AccumulateVertexDensity(const InputMesh &inputMesh, int n)
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

    std::vector<float> SampleAdaptively(const std::vector<float> &cdf, int n)
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
            auto cdfu = AccumulateVertexDensity<0>(inputMesh, res.x * 3);
            us = SampleAdaptively(cdfu, res.x);

            auto cdfv = AccumulateVertexDensity<1>(inputMesh, res.y * 3);
            vs = SampleAdaptively(cdfv, res.y);
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
        return device->CreateAndUploadTexture2D(RHI::TextureDesc
        {
            .format = RHI::Format::R32G32_Float,
            .width = uvData.GetWidth(),
            .height = uvData.GetHeight(),
            .usage = RHI::TextureUsage::ShaderResource
        }, uvData.GetData(), RHI::TextureLayout::ShaderTexture);
    }

} // namespace GridAlignment
