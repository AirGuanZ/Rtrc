#pragma once

#include "GridAlignment.h"

struct InputMesh;

class VDMBaker
{
public:

    explicit VDMBaker(DeviceRef device);

    RC<StatefulTexture> BakeUniformVDM(
        GraphRef         graph,
        const InputMesh &inputMesh,
        const Vector2u  &res);

    RC<StatefulTexture> BakeVDMBySeamCarving(
        GraphRef         graph,
        const InputMesh &inputMesh,
        const Vector2u  &initialSampleResolution,
        const Vector2u  &res);

    RC<StatefulTexture> BakeAlignedVDM(
        GraphRef                               graph,
        const InputMesh                       &inputMesh,
        const Image<GridAlignment::GridPoint> &grid,
        const RC<Texture>                     &gridUVTexture);


private:

    DeviceRef device_;
    GraphicsPipelineCache pipelineCache_;
};
