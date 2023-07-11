#pragma once

struct LocalFrame
{
    float3 x, y, z;

    void InitializeFromNormalizedX(float3 nx)
    {
        float3 t = abs(abs(nx.y) - 1) < 0.1 ? float3(0, 0, 1) : float3(0, 1, 0);
        x = nx;
        z = normalize(cross(x, t));
        y = cross(z, x);
    }

    void InitializeFromNormalizedY(float3 ny)
    {
        float3 t = abs(abs(ny.z) - 1) < 0.1 ? float3(1, 0, 0) : float3(0, 0, 1);
        y = ny;
        x = normalize(cross(y, t));
        z = cross(x, y);
    }

    void InitializeFromNormalizedZ(float3 nz)
    {
        float3 t = abs(abs(nz.x) - 1) < 0.1 ? float3(0, 1, 0) : float3(1, 0, 0);
        z = nz;
        y = normalize(cross(z, t));
        x = cross(y, z);
    }

    float3 LocalToGlobal(float3 localVec)
    {
        return x * localVec.x + y * localVec.y + z * localVec.z;
    }

    float3 GlobalToLocal(float3 globalVec)
    {
        return float3(dot(globalVec, x), dot(globalVec, y), dot(globalVec, z));
    }
};
