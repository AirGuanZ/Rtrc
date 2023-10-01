#pragma once

#include <Graphics/Device/Device.h>
#include <Rtrc/Resource/Mesh/Mesh.h>
#include <Rtrc/Scene/Camera/Camera.h>

RTRC_BEGIN

/*
  Fullscreen triangle
      uv(0, 0)
       *---------*
       |    |   /
       |    | /
       |----/ uv(1, 1)
       |  /
       |/
       *
    struct Vertex
    {
        float2 position  : POSITION;
        float2 uv        : UV;
        float3 worldRay  : WORLD_RAY;
        float3 cameraRay : CAMERA_RAY;
    };

  Fullscreen quad
      uv(0, 0)
      *-------*
      |     / |
      |   /   |
      | /     |
      *-------*
             uv(1, 1)
*/

const MeshLayout *GetFullscreenPrimitiveMeshLayout();
const MeshLayout *GetFullscreenPrimitiveMeshLayoutWithWorldRay();

Mesh GetFullscreenTriangle(DynamicBufferManager &bufferManager);
Mesh GetFullscreenTriangle(DynamicBufferManager &bufferManager, const Camera &camera);
Mesh GetFullscreenTriangle(DynamicBufferManager &bufferManager, const Camera &camera);

Mesh GetFullscreenQuad(DynamicBufferManager &bufferManager);
Mesh GetFullscreenQuad(DynamicBufferManager &bufferManager, const Camera &camera);
Mesh GetFullscreenQuad(DynamicBufferManager &bufferManager, const Camera &camera);

RTRC_END
