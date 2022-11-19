#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Renderer/Camera.h>

RTRC_BEGIN

RC<Mesh> GetFullscreenTriangleMesh(DynamicBufferManager &bufferManager);
RC<Mesh> GetFullscreenTriangleMesh(DynamicBufferManager &bufferManager, const RenderCamera &camera);

RTRC_END
