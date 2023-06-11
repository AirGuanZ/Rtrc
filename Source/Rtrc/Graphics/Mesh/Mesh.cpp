#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Graphics/Device/CommandBuffer.h>

RTRC_BEGIN

const VertexBufferLayout *Mesh::BuiltinVertexBufferLayout::Default = RTRC_VERTEX_BUFFER_LAYOUT(
    Attribute("POSITION", 0, Float3),
    Attribute("NORMAL",   0, Float3),
    Attribute("TEXCOORD", 0, Float2),
    Attribute("TANGENT",  0, Float3));

const VertexBufferLayout *Mesh::BuiltinVertexBufferLayout::Simplified = RTRC_VERTEX_BUFFER_LAYOUT(
    Attribute("POSITION", 0, Float3),
    Attribute("NORMAL",   0, Float3),
    Attribute("TEXCOORD", 0, Float2));

void Mesh::SharedRenderingData::BindVertexAndIndexBuffers(CommandBuffer &commandBuffer) const
{
    commandBuffer.SetVertexBuffers(0, vertexBuffers_);
    if(indexBuffer_)
    {
        commandBuffer.SetIndexBuffer(indexBuffer_, indexFormat_);
    }
}

RTRC_END
