#include "SeamEnergy.h"

namespace SeamEnergy
{

    Image<Vector4f> ComputePositionMap(const Image<Vector4f> &vdm, const Rectf &gridRect)
    {
        Image<Vector4f> ret(vdm.GetWidth(), vdm.GetHeight());
        ParallelFor(0, vdm.GetSHeight(), [&](int y)
        {
            const float ty = static_cast<float>(y) / static_cast<float>(vdm.GetSHeight() - 1);
            const float oy = std::lerp(gridRect.lower.y, gridRect.upper.y, ty);
            for(int x = 0; x < vdm.GetSWidth(); ++x)
            {
                const float tx = static_cast<float>(x) / static_cast<float>(vdm.GetSWidth() - 1);
                const float ox = std::lerp(gridRect.lower.x, gridRect.upper.x, tx);
                const Vector3f displacement = vdm(x, y).xyz();
                const Vector3f position = Vector3f(ox, 0, oy) + displacement;
                ret(x, y) = Vector4f(position, 0);
            }
        });
        return ret;
    }

    Image<Vector4f> ComputeDisplacementMap(const Image<Vector4f> &posMap, const Rectf &gridRect)
    {
        Image<Vector4f> ret(posMap.GetWidth(), posMap.GetHeight());
        ParallelFor(0, ret.GetSHeight(), [&](int y)
        {
            const float ty = static_cast<float>(y) / static_cast<float>(ret.GetSHeight() - 1);
            const float oy = std::lerp(gridRect.lower.y, gridRect.upper.y, ty);
            for(int x = 0; x < ret.GetSWidth(); ++x)
            {
                const float tx = static_cast<float>(x) / static_cast<float>(ret.GetSWidth() - 1);
                const float ox = std::lerp(gridRect.lower.x, gridRect.upper.x, tx);
                const Vector3f position = posMap(x, y).xyz();
                const Vector3f displacement = position - Vector3f(ox, 0, oy);
                ret(x, y) = Vector4f(displacement, 0);
            }
        });
        return ret;
    }

    Image<Vector4f> ComputeNormalMap(const Image<Vector4f> &posMap)
    {
        Image<Vector4f> ret(posMap.GetWidth(), posMap.GetHeight());
        auto GetPosition = [&, sx = posMap.GetSWidthMinusOne(), sy = posMap.GetSHeightMinusOne()](int x, int y)
        {
            x = std::clamp(x, 0, sx);
            y = std::clamp(y, 0, sy);
            return posMap(x, y).xyz();
        };

        ParallelFor(0, ret.GetSHeight(), [&](int y)
        {
            for(int x = 0; x < ret.GetSWidth(); ++x)
            {
                const Vector3f dpdu =
                    1.0f / 4.0f * (GetPosition(x + 1, y - 1) - GetPosition(x - 1, y - 1)) +
                    1.0f / 2.0f * (GetPosition(x + 1, y + 0) - GetPosition(x - 1, y + 0)) +
                    1.0f / 4.0f * (GetPosition(x + 1, y + 1) - GetPosition(x - 1, y + 1));
                const Vector3f dpdv =
                    1.0f / 4.0f * (GetPosition(x - 1, y + 1) - GetPosition(x - 1, y - 1)) +
                    1.0f / 2.0f * (GetPosition(x + 0, y + 1) - GetPosition(x + 0, y - 1)) +
                    1.0f / 4.0f * (GetPosition(x + 1, y + 1) - GetPosition(x + 1, y - 1));
                const Vector3f normal = Normalize(Cross(dpdv, dpdu));
                ret(x, y) = Vector4f(normal, 0);
            }
        });

        return ret;
    }

    Image<float> ComputeEnergyMapBasedOnNormalDifference(const Image<Vector4f> &posMap)
    {
        auto normalMap = ComputeNormalMap(posMap);
        auto GetNormal = [&, sx = posMap.GetSWidthMinusOne(), sy = posMap.GetSHeightMinusOne()](int x, int y)
        {
            x = std::clamp(x, 0, sx);
            y = std::clamp(y, 0, sy);
            return normalMap(x, y);
        };

        Image<float> ret(posMap.GetWidth(), posMap.GetHeight());
        ParallelFor(0, posMap.GetSHeight(), [&](int y)
        {
            for(int x = 0; x < posMap.GetSWidth(); ++x)
            {
                auto F = [](float x) { return (1 - x) * (1 - x); };
                const Vector4f &dc = GetNormal(x, y);
                const Vector4f &d0 = GetNormal(x + 1, y);
                const Vector4f &d1 = GetNormal(x - 1, y);
                const Vector4f &d2 = GetNormal(x, y + 1);
                const Vector4f &d3 = GetNormal(x, y - 1);
                const float diff = F(Dot(d0, dc)) + F(Dot(d1, dc)) + F(Dot(d2, dc)) + F(Dot(d3, dc));
                ret(x, y) = std::sqrt(diff);
            }
        });

        return ret;
    }

    Image<float> ComputeEnergyMapBasedOnInterpolationError(const Image<Vector4f> &posMap, bool interpolateAlongX)
    {
        const int w = posMap.GetSWidth(), h = posMap.GetSHeight();
        Image<float> ret(w, h);
        ParallelFor(0, posMap.GetSHeight(), [&](int y)
        {
            for(int x = 0; x < posMap.GetSWidth(); ++x)
            {
                const Vector3f originalPosition = posMap(x, y).xyz();
                float error;
                if(interpolateAlongX)
                {
                    if(x == 0 || x == w - 1)
                    {
                        const int nx = x == 0 ? 1 : w - 2;
                        const Vector3f interpolatedPosition = posMap(nx, y).xyz();
                        error = Length(originalPosition - interpolatedPosition);
                    }
                    else
                    {
                        const Vector3f a = posMap(x - 1, y).xyz();
                        const Vector3f b = posMap(x + 1, y).xyz();
                        const Vector3f c = originalPosition;
                        const Vector3f n = Normalize(b - a);
                        const Vector3f proj = n * Dot(c - a, n);
                        error = Length(c - a - proj);
                    }
                }
                else
                {
                    if(y == 0 || y == h - 1)
                    {
                        const int ny = y == 0 ? 1 : h - 2;
                        const Vector3f interpolatedPosition = posMap(x, ny).xyz();
                        error = Length(originalPosition - interpolatedPosition);
                    }
                    else
                    {
                        const Vector3f a = posMap(x, y - 1).xyz();
                        const Vector3f b = posMap(x, y + 1).xyz();
                        const Vector3f c = originalPosition;
                        const Vector3f n = Normalize(b - a);
                        const Vector3f proj = n * Dot(c - a, n);
                        error = Length(c - a - proj);
                    }
                }
                ret(x, y) = error;
            }
        });
        return ret;
    }

    Image<float> ComputeVerticalDPTable(const Image<float> &energyMap)
    {
        const int w = energyMap.GetSWidth(), h = energyMap.GetSHeight();
        Image<float> ret(w, h);

        for(int x = 0; x < w; ++x)
        {
            ret(x, h - 1) = energyMap(x, h - 1);
        }

        for(int y = h - 2; y >= 0; --y)
        {
            for(int x = 0; x < w; ++x)
            {
                const float e0 = x > 1 ? ret(x - 1, y + 1) : 999999.0f;
                const float e1 = ret(x, y + 1);
                const float e2 = x + 1 < w - 1 ? ret(x + 1, y + 1) : 999999.0f;
                const float de = energyMap(x, y);
                ret(x, y) = de + (std::min)({ e0, e1, e2 });
            }
        }

        return ret;
    }

    Image<float> ComputeHorizontalDPTable(const Image<float>& energyMap)
    {
        const int w = energyMap.GetSWidth(), h = energyMap.GetSHeight();
        Image<float> ret(w, h);

        for(int y = 0; y < h; ++y)
        {
            ret(w - 1, y) = energyMap(w - 1, y);
        }

        for(int x = w - 2; x >= 0; --x)
        {
            for(int y = 0; y < h; ++y)
            {
                const float e0 = y > 1 ? ret(x + 1, y - 1) : 999999.0f;
                const float e1 = ret(x + 1, y);
                const float e2 = y + 1 < h - 1 ? ret(x + 1, y + 1) : 999999.0f;
                const float de = energyMap(x, y);
                ret(x, y) = de + (std::min)({ e0, e1, e2 });
            }
        }

        return ret;
    }

    std::vector<int> GetHorizontalSeam(const Image<float>& dpTable, int entryY)
    {
        std::vector<int> ret(dpTable.GetSWidth());
        ret[0] = entryY;

        int currY = entryY;
        for(int x = 1; x < dpTable.GetSWidth(); ++x)
        {
            int nextY = currY;
            auto Update = [&, minE = dpTable(x, currY)](int y) mutable
            {
                if(const float e = dpTable(x, y); e < minE)
                {
                    nextY = y;
                    minE = e;
                }
            };
            {
                if(currY > 1)
                {
                    Update(currY - 1);
                }
                if(currY + 1 < dpTable.GetSHeight() - 1)
                {
                    Update(currY + 1);
                }
            }
            ret[x] = nextY;
            currY = nextY;
        }

        return ret;
    }

    std::vector<int> GetVerticalSeam(const Image<float>& dpTable, int entryX)
    {
        std::vector<int> ret(dpTable.GetSHeight());
        ret[0] = entryX;

        int currX = entryX;
        for(int y = 1; y < dpTable.GetSHeight(); ++y)
        {
            int nextX = currX;
            auto Update = [&, minE = dpTable(currX, y)](int x) mutable
            {
                if(const float e = dpTable(x, y); e < minE)
                {
                    nextX = x;
                    minE = e;
                }
            };
            {
                if(currX > 1)
                {
                    Update(currX - 1);
                }
                if(currX + 1 < dpTable.GetSWidth() - 1)
                {
                    Update(currX + 1);
                }
            }
            ret[y] = nextX;
            currX = nextX;
        }

        return ret;
    }

    Image<Vector4f> RemoveHorizontalSeam(const Image<Vector4f>& posMap, const std::vector<int>& seam)
    {
        Image<Vector4f> ret(posMap.GetWidth(), posMap.GetHeight() - 1);
        ParallelFor(0, posMap.GetSWidth(), [&](int x)
        {
            const int seamY = seam[x];
            for(int y = 0; y < seamY; ++y)
            {
                ret(x, y) = posMap(x, y);
            }
            for(int y = seamY + 1; y < posMap.GetSHeight(); ++y)
            {
                ret(x, y - 1) = posMap(x, y);
            }
        });
        return ret;
    }

    Image<Vector4f> RemoveVerticalSeam(const Image<Vector4f>& posMap, const std::vector<int>& seam)
    {
        Image<Vector4f> ret(posMap.GetWidth() - 1, posMap.GetHeight());
        ParallelFor(0, posMap.GetSHeight(), [&](int y)
        {
            const int seamX = seam[y];
            if(seamX > 0)
            {
                std::memcpy(&ret(0, y), &posMap(0, y), seamX * sizeof(Vector4f));
            }
            if(seamX + 1 < posMap.GetSWidth())
            {
                std::memcpy(&ret(seamX, y), &posMap(seamX + 1, y), (posMap.GetSWidth() - 1 - seamX) * sizeof(Vector4f));
            }
        });
        return ret;
    }

} // namespace SeamEnergy
