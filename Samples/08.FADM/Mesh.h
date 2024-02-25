#pragma once

#include "Common.h"

struct SharpEdge
{
    int v0;
    int v1;
};

struct SharpSegment
{
    std::vector<int> edges;
};

struct SharpFeatures
{
    std::vector<SharpEdge> edges;
    std::vector<SharpSegment> segments;
    std::vector<int> corners;
    std::vector<float> vertexImportance;
};

struct InputMesh
{
    std::vector<Vector3f> positions;
    std::vector<Vector2f> parameterSpacePositions;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> sharpEdgeIndices;

    SharpFeatures sharpFeatures;

    Vector2f gridLower;
    Vector2f gridUpper;

    Vector3f lower;
    Vector3f upper;

    RC<Buffer> positionBuffer;
    RC<Buffer> parameterSpacePositionBuffer;
    RC<Buffer> indexBuffer;
    RC<Buffer> sharpEdgeIndexBuffer;

    // Fill positions, parameterSpacePositions and indices
    static InputMesh Load(const std::string &filename);

    void DetectSharpFeatures(float angleThreshold);

    // Fill *Buffer
    void Upload(DeviceRef device);
};
