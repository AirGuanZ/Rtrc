#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Resource/MeshManager.h>
#include <Rtrc/Math/Frame.h>
#include <Rtrc/Utility/MeshData.h>
#include <Rtrc/Utility/String.h>

RTRC_BEGIN

size_t MeshManager::Options::Hash() const
{
    return (generateTangentIfNotPresent ? 1 : 0) |
           (noIndexBuffer ? 2 : 0);
}

Mesh MeshManager::Load(Device *device, const std::string &filename, const Options &options)
{
    MeshData meshData = MeshData::LoadFromObjFile(filename);

    // Remove index data when tangent vectors are required. Theoretically we only need to do this when
    // tangents of different faces don't match at the same vertex.
    // TODO: Optimize this
    if((options.noIndexBuffer || options.generateTangentIfNotPresent) && !meshData.indexData.empty())
    {
        meshData = meshData.RemoveIndexData();
    }

    std::vector<Vector3f> tangentData;
    if(options.generateTangentIfNotPresent)
    {
        assert(meshData.positionData.size() % 3 == 0);
        tangentData.resize(meshData.positionData.size());
        for(size_t i = 0, j = 0; j < meshData.positionData.size(); ++i, j += 3)
        {
            const Vector3f a = meshData.positionData[j + 0];
            const Vector3f b = meshData.positionData[j + 1];
            const Vector3f c = meshData.positionData[j + 2];
            const Vector3f na = meshData.normalData[j + 0];
            const Vector3f nb = meshData.normalData[j + 1];
            const Vector3f nc = meshData.normalData[j + 2];
            const Vector2f ta = meshData.texCoordData[j + 0];
            const Vector2f tb = meshData.texCoordData[j + 1];
            const Vector2f tc = meshData.texCoordData[j + 2];
            const Vector3f nor = Normalize(na + nb + nc);
            const Vector3f tangent = ComputeTangent(b - a, c - a, tb - ta, tc - ta, nor);
            tangentData[j + 0] = tangent;
            tangentData[j + 1] = tangent;
            tangentData[j + 2] = tangent;
        }
    }

    auto FillVertexData = [&]<typename Vertex>(Vertex &vertex, size_t i)
    {
        vertex.position = meshData.positionData[i];
        vertex.normal = meshData.normalData[i];
        vertex.texCoord = meshData.texCoordData[i];
        if constexpr(requires{ typename Vertex::tangent; })
        {
            assert(!tangentData.empty());
            vertex.tangent = tangentData[i];
        }
    };

    MeshBuilder meshBuilder;
    
    if(tangentData.empty())
    {
        meshBuilder.SetLayout(RTRC_MESH_LAYOUT(Buffer(
            Attribute("POSITION", Float3),
            Attribute("NORMAL", Float3),
            Attribute("TEXCOORD", Float2))));

        struct Vertex
        {
            Vector3f position;
            Vector3f normal;
            Vector2f texCoord;
        };
        std::vector<Vertex> vertexData;
        vertexData.resize(meshData.positionData.size());
        for(size_t i = 0; i < meshData.positionData.size(); ++i)
        {
            FillVertexData(vertexData[i], i);
        }
        meshBuilder.SetVertexCount(static_cast<uint32_t>(vertexData.size()));

        auto vertexBuffer = device->GetCopyContext().CreateBuffer(
            RHI::BufferDesc{
                sizeof(Vertex) * vertexData.size(),
                RHI::BufferUsage::VertexBuffer,
                RHI::BufferHostAccessType::None
            },
            vertexData.data());
        meshBuilder.SetVertexBuffer(0, vertexBuffer);
    }
    else
    {
        meshBuilder.SetLayout(RTRC_MESH_LAYOUT(Buffer(
            Attribute("POSITION", Float3),
            Attribute("NORMAL", Float3),
            Attribute("TEXCOORD", Float2),
            Attribute("TANGENT", Float3))));

        struct Vertex
        {
            Vector3f position;
            Vector3f normal;
            Vector2f texCoord;
            Vector3f tangent;
        };
        std::vector<Vertex> vertexData;
        vertexData.resize(meshData.positionData.size());
        for(size_t i = 0; i < meshData.positionData.size(); ++i)
        {
            FillVertexData(vertexData[i], i);
        }
        meshBuilder.SetVertexCount(static_cast<uint32_t>(vertexData.size()));
        
        auto vertexBuffer = device->GetCopyContext().CreateBuffer(
            RHI::BufferDesc{
                sizeof(Vertex) * vertexData.size(),
                RHI::BufferUsage::VertexBuffer,
                RHI::BufferHostAccessType::None
            },
            vertexData.data());
        meshBuilder.SetVertexBuffer(0, vertexBuffer);
    }

    if(!meshData.indexData.empty())
    {
        bool useUInt16 = true;
        for(uint32_t index : meshData.indexData)
        {
            if(index > (std::numeric_limits<uint16_t>::max)())
            {
                useUInt16 = false;
                break;
            }
        }

        RC<Buffer> indexBuffer;
        if(useUInt16)
        {
            std::vector<uint16_t> uint16IndexData;
            uint16IndexData.resize(meshData.indexData.size());
            for(size_t i = 0; i < meshData.indexData.size(); ++i)
            {
                uint16IndexData[i] = static_cast<uint16_t>(meshData.indexData[i]);
            }
            indexBuffer = device->GetCopyContext().CreateBuffer(
                RHI::BufferDesc{
                    sizeof(uint16_t) * uint16IndexData.size(),
                    RHI::BufferUsage::IndexBuffer,
                    RHI::BufferHostAccessType::None
                },
                uint16IndexData.data());
        }
        else
        {
            indexBuffer = device->GetCopyContext().CreateBuffer(
                RHI::BufferDesc{
                    sizeof(uint32_t) * meshData.indexData.size(),
                    RHI::BufferUsage::IndexBuffer,
                    RHI::BufferHostAccessType::None
                },
                meshData.indexData.data());
        }

        meshBuilder.SetIndexBuffer(std::move(indexBuffer), RHI::IndexBufferFormat::UInt16);
        meshBuilder.SetIndexCount(static_cast<uint32_t>(meshData.indexData.size()));
    }

    return meshBuilder.CreateMesh();
}

MeshManager::MeshManager()
    : device_(nullptr)
{
    
}

void MeshManager::SetDevice(Device *device)
{
    device_ = device;
}

RC<Mesh> MeshManager::GetMesh(std::string_view name, const Options &options)
{
    const std::string filename = absolute(std::filesystem::path(name)).lexically_normal().string();
    return meshCache_.GetOrCreate(std::make_pair(filename, options), [&]
    {
        return Load(device_, filename, options);
    });
}

Vector3f MeshManager::ComputeTangent(
    const Vector3f &B_A, const Vector3f &C_A, const Vector2f &b_a, const Vector2f &c_a, const Vector3f &nor)
{
    const float m00 = b_a.x, m01 = b_a.y;
    const float m10 = c_a.x, m11 = c_a.y;
    const float det = m00 * m11 - m01 * m10;
    if(det == 0.0f)
    {
        return Frame::FromZ(nor).x;
    }
    const float invDet = 1 / det;
    return Normalize(m11 * invDet * B_A - m01 * invDet * C_A);
}

bool operator<(
    const std::pair<std::string, MeshManager::Options> &lhs,
    const std::pair<std::string_view, MeshManager::Options> &rhs)
{
    return std::make_pair(std::string_view(lhs.first), lhs.second) < rhs;
}

bool operator<(
    const std::pair<std::string_view, MeshManager::Options> &lhs,
    const std::pair<std::string, MeshManager::Options> &rhs)
{
    return lhs < std::make_pair(std::string_view(rhs.first), rhs.second);
}

RTRC_END