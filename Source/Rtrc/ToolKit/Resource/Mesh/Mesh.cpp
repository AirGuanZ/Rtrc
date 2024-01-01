#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/ToolKit/Resource/Mesh/Mesh.h>

RTRC_BEGIN

const VertexBufferLayout *Mesh::BuiltinVertexBufferLayout::Standard = RTRC_VERTEX_BUFFER_LAYOUT(
    Attribute("POSITION", 0, Float3),
    Attribute("NORMAL",   0, Float3),
    Attribute("TEXCOORD", 0, Float2),
    Attribute("TANGENT",  0, Float3));

void Mesh::Bind(CommandBuffer &commandBuffer) const
{
    commandBuffer.SetVertexBuffers(0, vertexBuffers_, vertexStrides_);
    if(indexBuffer_)
    {
        commandBuffer.SetIndexBuffer(indexBuffer_, indexFormat_);
    }
}

RTRC_END
