#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/ToolKit/Resource/Mesh/Mesh.h>

RTRC_BEGIN

void Mesh::Bind(CommandBuffer &commandBuffer) const
{
    commandBuffer.SetVertexBuffers(0, vertexBuffers_, vertexStrides_);
    if(indexBuffer_)
    {
        commandBuffer.SetIndexBuffer(indexBuffer_, indexFormat_);
    }
}

RTRC_END
