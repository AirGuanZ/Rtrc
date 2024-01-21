#include <tiny_obj_loader.h>

#include <Rtrc/Core/Container/StaticVector.h>
#include <Rtrc/Core/Resource/MeshData.h>

RTRC_BEGIN

namespace MeshDataDetail
{

    template<bool WantNormal, bool WantTexCoord>
    MeshData LoadFromObjFile(const std::string &filename)
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
                const int lhsNormalIndex = WantNormal ? lhs.normal_index : 0;
                const int rhsNormalIndex = WantNormal ? rhs.normal_index : 0;
                const int lhsTexCoordIndex = WantTexCoord ? lhs.texcoord_index : 0;
                const int rhsTexCoordIndex = WantTexCoord ? rhs.texcoord_index : 0;
                return std::make_tuple(lhs.vertex_index, lhsNormalIndex, lhsTexCoordIndex) <
                       std::make_tuple(rhs.vertex_index, rhsNormalIndex, rhsTexCoordIndex);
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

            if constexpr(WantNormal)
            {
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
            }

            if constexpr(WantTexCoord)
            {
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

                if constexpr(WantNormal)
                {
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
        }

        return meshData;
    }

} // namespace MeshDataDetail

MeshData MeshData::LoadFromObjFile(const std::string &filename, bool wantNormal, bool wantTexCoord)
{
    if(wantNormal)
    {
        if(wantTexCoord)
        {
            return MeshDataDetail::LoadFromObjFile<true, true>(filename);
        }
        return MeshDataDetail::LoadFromObjFile<true, false>(filename);
    }

    if(wantTexCoord)
    {
        return MeshDataDetail::LoadFromObjFile<false, true>(filename);
    }
    return MeshDataDetail::LoadFromObjFile<false, false>(filename);
}

std::vector<size_t> MeshData::MergeIdenticalPositions()
{
    std::vector<size_t> mapping(positionData.size());

    MeshData mergedMesh;
    std::map<Vector3f, size_t> positionToIndex;
    auto GetIndex = [&](const Vector3f &position, size_t oldPositionIndex)
    {
        if(auto it = positionToIndex.find(position); it != positionToIndex.end())
        {
            mapping[oldPositionIndex] = it->second;
            return it->second;
        }
        const size_t newIndex = mergedMesh.positionData.size();
        mergedMesh.positionData.push_back(position);
        positionToIndex.insert({ position, newIndex });
        mapping[oldPositionIndex] = newIndex;
        return newIndex;
    };

    for(auto oldIndex : indexData)
    {
        mergedMesh.indexData.push_back(GetIndex(positionData[oldIndex], oldIndex));
    }

    *this = mergedMesh;
    return mapping;
}

void MeshData::RemoveIndexData()
{
    if(indexData.empty())
    {
        return;
    }

    MeshData result;
    result.positionData.reserve(indexData.size());
    if(!normalData.empty())
    {
        result.normalData.reserve(indexData.size());
    }
    if(!texCoordData.empty())
    {
        result.texCoordData.reserve(indexData.size());
    }

    assert(indexData.size() % 3 == 0);
    for(size_t i = 0; i < indexData.size(); ++i)
    {
        const uint32_t index = indexData[i];
        result.positionData.push_back(positionData[index]);
        if(!normalData.empty())
        {
            result.normalData.push_back(normalData[index]);
        }
        if(!texCoordData.empty())
        {
            result.texCoordData.push_back(texCoordData[index]);
        }
    }

    *this = result;
}

RTRC_END
