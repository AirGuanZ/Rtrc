#include <tiny_obj_loader.h>

#include <Rtrc/Utility/MeshData.h>

RTRC_BEGIN

MeshData MeshData::LoadFromObjFile(const std::string &filename)
{
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;
    reader_config.vertex_color = false;
    if(!reader.ParseFromFile(filename, reader_config))
    {
        throw Exception(reader.Error());
    }

    struct CompareIndex
    {
        bool operator()(const tinyobj::index_t &lhs, const tinyobj::index_t &rhs) const
        {
            return std::make_tuple(lhs.vertex_index, lhs.normal_index, lhs.texcoord_index) <
                   std::make_tuple(rhs.vertex_index, rhs.normal_index, rhs.texcoord_index);
        }
    };
    std::map<tinyobj::index_t, uint32_t, CompareIndex> indexCache;

    MeshData meshData;
    auto &attribs = reader.GetAttrib();
    auto getIndex = [&](const tinyobj::index_t &i)
    {
        if(auto it = indexCache.find(i); it != indexCache.end())
        {
            return it->second;
        }
        const uint32_t newIndex = static_cast<uint32_t>(meshData.positionData.size());
        meshData.positionData.emplace_back(
            attribs.vertices[i.vertex_index * 3 + 0],
            attribs.vertices[i.vertex_index * 3 + 1],
            attribs.vertices[i.vertex_index * 3 + 2]);
        if(i.normal_index >= 0)
        {
            meshData.normalData.emplace_back(
                attribs.normals[i.normal_index * 3 + 0],
                attribs.normals[i.normal_index * 3 + 1],
                attribs.normals[i.normal_index * 3 + 2]);
        }
        else
        {
            meshData.normalData.emplace_back();
        }
        if(i.texcoord_index >= 0)
        {
            meshData.texCoordData.emplace_back(
                attribs.texcoords[i.texcoord_index * 2 + 0],
                attribs.texcoords[i.texcoord_index * 2 + 1]);
        }
        else
        {
            meshData.texCoordData.emplace_back();
        }
        indexCache.insert({ i, newIndex });
        return newIndex;
    };

    for(auto &shape : reader.GetShapes())
    {
        assert(shape.mesh.indices.size() % 3 == 0);
        const size_t triangleCount = shape.mesh.indices.size() / 3;
        for(size_t i = 0, j = 0; i < triangleCount; ++i, j += 3)
        {
            const tinyobj::index_t *triangleIndices = &shape.mesh.indices[j];

            const uint32_t indices[3] =
            {
                getIndex(triangleIndices[0]),
                getIndex(triangleIndices[1]),
                getIndex(triangleIndices[2])
            };

            meshData.indexData.push_back(indices[0]);
            meshData.indexData.push_back(indices[1]);
            meshData.indexData.push_back(indices[2]);

            if(triangleIndices[0].normal_index < 0 ||
               triangleIndices[1].normal_index < 0 ||
               triangleIndices[2].normal_index < 0)
            {
                const Vector3f &a = meshData.positionData[indices[0]];
                const Vector3f &b = meshData.positionData[indices[1]];
                const Vector3f &c = meshData.positionData[indices[2]];
                const Vector3f n = Normalize(Cross(c - b, b - a));
                meshData.normalData[indices[0]] = n;
                meshData.normalData[indices[1]] = n;
                meshData.normalData[indices[2]] = n;
            }
        }
    }

    return meshData;
}

MeshData MeshData::RemoveIndexData() const
{
    if(indexData.empty())
    {
        return *this;
    }
    MeshData ret;
    ret.positionData.reserve(indexData.size());
    ret.normalData.reserve(indexData.size());
    ret.texCoordData.reserve(indexData.size());
    assert(indexData.size() % 3 == 0);
    for(size_t i = 0; i < indexData.size(); ++i)
    {
        const uint32_t index = indexData[i];
        ret.positionData.push_back(positionData[index]);
        ret.normalData.push_back(normalData[index]);
        ret.texCoordData.push_back(texCoordData[index]);
    }
    return ret;
}

RTRC_END
