#pragma once

#include "Mesh.h"

namespace GridAlignment
{
    struct GridPoint
    {
        int corner = -1;
        int edge = -1;
        Vector2f referenceUV;
        Vector2f uv;
    };

    Image<GridPoint> CreateGrid(const Vector2u &res, const InputMesh &inputMesh, bool adaptive);

    void AlignCorners(Image<GridPoint> &grid, const InputMesh &inputMesh);

    void AlignSegments(Image<GridPoint> &grid, const InputMesh &inputMesh);

    RC<Texture> UploadGrid(DeviceRef device, const Image<GridPoint> &grid);

} // namespace GridAlignment
