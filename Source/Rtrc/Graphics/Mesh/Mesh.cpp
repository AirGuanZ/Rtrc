#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Graphics/Device/CommandBuffer.h>

RTRC_BEGIN

void Mesh::Bind(CommandBuffer &commandBuffer) const
{
    commandBuffer.SetVertexBuffers(0, vertexBuffers_, vertexBufferByteOffsets_);
    if(indexBuffer_)
    {
        commandBuffer.SetIndexBuffer(indexBuffer_, indexFormat_, indexBufferByteOffset_);
    }
}

RTRC_END
