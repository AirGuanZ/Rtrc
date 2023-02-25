#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Graphics/Device/CommandBuffer.h>

RTRC_BEGIN

void Mesh::SharedRenderingData::Bind(CommandBuffer &commandBuffer) const
{
    commandBuffer.SetVertexBuffers(0, vertexBuffers_);
    if(indexBuffer_)
    {
        commandBuffer.SetIndexBuffer(indexBuffer_, indexFormat_);
    }
}

RTRC_END
