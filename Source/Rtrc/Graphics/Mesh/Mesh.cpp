#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Graphics/Object/CommandBuffer.h>

RTRC_BEGIN

void Mesh::Bind(CommandBuffer &commandBuffer) const
{
    commandBuffer.SetVertexBuffers(0, vertexBuffers_);
    if(indexBuffer_)
    {
        commandBuffer.SetIndexBuffer(indexBuffer_, indexBufferFormat_);
    }
}

RTRC_END
