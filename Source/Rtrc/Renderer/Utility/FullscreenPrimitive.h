#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Renderer/Camera.h>

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
*/

RC<Mesh> GetFullscreenTriangle(DynamicBufferManager &bufferManager);
RC<Mesh> GetFullscreenTriangle(DynamicBufferManager &bufferManager, const RenderCamera &camera);

RTRC_END
