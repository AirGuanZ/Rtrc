#include <fstream>

#include <tiny_obj_loader.h>

#include <Rtrc/Core/String.h>
#include <Rtrc/Geometry/RawMesh.h>

RTRC_GEO_BEGIN

RawMesh RawMesh::Load(const std::string& filename)
{
    const std::string_view ext = GetExtension(filename);
    if(ToLower(ext) == ".obj")
    {
        return LoadWavefrontObj(filename);
    }
    throw Exception(fmt::format("Unsupported mesh file extension: '{}'", ext));
}

void RawMesh::SetTriangleCount(uint32_t count)
{
    assert(attributes_.empty());
    assert(count > 0);
    triangleCount_ = count;
}

void RawMesh::AddAttribute(
    std::string                name,
    RawMeshAttributeData::Type type,
    std::vector<unsigned char> data,
    std::vector<uint32_t>      indices)
{
    assert(triangleCount_);
    assert(!nameToAttributeIndex_.contains(name));
    assert(data.size() % RawMeshDetail::GetAttributeBytes(type) == 0);
    assert(indices.size() % 3 == 0);
    assert(indices.size() / 3 == triangleCount_);

    const int attributeIndex = static_cast<int>(attributes_.size());
    auto &attribute = attributes_.emplace_back();
    attribute.type_ = type;
    attribute.name_ = std::move(name);
    attribute.data_ = std::move(data);

    indices_.push_back(std::move(indices));

    if(attribute.name_ == "position")
    {
        assert(type == RawMeshAttributeData::Type::Float3);
        builtinAttributeIndices_[std::to_underlying(BuiltinAttribute::Position)] = attributeIndex;
    }
    else if(attribute.name_ == "normal")
    {
        assert(type == RawMeshAttributeData::Type::Float3);
        builtinAttributeIndices_[std::to_underlying(BuiltinAttribute::Normal)] = attributeIndex;
    }
    else if(attribute.name_ == "uv")
    {
        assert(type == RawMeshAttributeData::Type::Float2);
        builtinAttributeIndices_[std::to_underlying(BuiltinAttribute::UV)] = attributeIndex;
    }
    else if(attribute.name_ == "color")
    {
        assert(type == RawMeshAttributeData::Type::Float3);
        builtinAttributeIndices_[std::to_underlying(BuiltinAttribute::Color)] = attributeIndex;
    }
}

std::string_view RawMesh::GetExtension(const std::string& filename)
{
    const size_t pos = filename.rfind('.');
    if(pos == std::string::npos)
    {
        throw Exception(fmt::format("No extension found in {}", filename));
    }
    return std::string_view(filename).substr(pos);
}

void RawMesh::RecalculateNormal(float cosAngleThreshold)
{
    auto positionData = GetPositionData();
    auto positionIndices = GetPositionIndices();

    std::vector<Vector3f> newNormals;
    std::vector<uint32_t> newNormalIndices;
    std::map<Vector3f, uint32_t> newNormalToIndex;
    auto AppendNewNormalIndex = [&](const Vector3f &newNormal)
    {
        auto it = newNormalToIndex.find(newNormal);
        if(it == newNormalToIndex.end())
        {
            const uint32_t newNormalIndex = static_cast<uint32_t>(newNormals.size());
            newNormals.push_back(newNormal);
            it = newNormalToIndex.emplace(std::make_pair(newNormal, newNormalIndex)).first;
        }
        newNormalIndices.push_back(it->second);
    };

    if(cosAngleThreshold >= 1)
    {
        for(uint32_t f0 = 0; f0 < positionIndices.size(); f0 += 3)
        {
            const uint32_t va = positionIndices[f0 + 0];
            const uint32_t vb = positionIndices[f0 + 1];
            const uint32_t vc = positionIndices[f0 + 2];

            const Vector3f &pa = positionData[va];
            const Vector3f &pb = positionData[vb];
            const Vector3f &pc = positionData[vc];

            const Vector3f faceNormal = Normalize(Cross(pc - pb, pb - pa));

            AppendNewNormalIndex(faceNormal);
            AppendNewNormalIndex(faceNormal);
            AppendNewNormalIndex(faceNormal);
        }
    }
    else
    {
        struct FaceNormalRecord
        {
            Vector3f normal;
            float angle;
        };
        std::vector<std::vector<FaceNormalRecord>> vertexToFaceNormals(positionData.size());

        for(uint32_t f0 = 0; f0 < positionIndices.size(); f0 += 3)
        {
            const uint32_t va = positionIndices[f0 + 0];
            const uint32_t vb = positionIndices[f0 + 1];
            const uint32_t vc = positionIndices[f0 + 2];

            const Vector3f &pa = positionData[va];
            const Vector3f &pb = positionData[vb];
            const Vector3f &pc = positionData[vc];

            const Vector3f faceNormal = Normalize(Cross(pc - pb, pb - pa));
            const float ca = Dot(Normalize(pb - pa), Normalize(pc - pa));
            const float cb = Dot(Normalize(pc - pb), Normalize(pa - pb));
            const float cc = Dot(Normalize(pa - pc), Normalize(pb - pc));

            vertexToFaceNormals[va].push_back({ faceNormal, std::acos(ca) });
            vertexToFaceNormals[vb].push_back({ faceNormal, std::acos(cb) });
            vertexToFaceNormals[vc].push_back({ faceNormal, std::acos(cc) });
        }

        auto ComputeAverageNormal = [&](const Vector3f &referenceNormal, Span<FaceNormalRecord> records)
        {
            Vector3f sumNormal(0, 0, 0);
            float sumWeight = 0;
            for(auto &record : records)
            {
                if(Dot(referenceNormal, record.normal) >= cosAngleThreshold)
                {
                    sumNormal += record.normal * record.angle;
                    sumWeight += record.angle;
                }
            }
            return sumNormal / (std::max)(sumWeight, 1e-5f);
        };

        for(uint32_t f0 = 0; f0 < positionIndices.size(); f0 += 3)
        {
            const uint32_t va = positionIndices[f0 + 0];
            const uint32_t vb = positionIndices[f0 + 1];
            const uint32_t vc = positionIndices[f0 + 2];

            const Vector3f &pa = positionData[va];
            const Vector3f &pb = positionData[vb];
            const Vector3f &pc = positionData[vc];
            const Vector3f faceNormal = Normalize(Cross(pc - pb, pb - pa));

            const Vector3f na = ComputeAverageNormal(faceNormal, vertexToFaceNormals[va]);
            const Vector3f nb = ComputeAverageNormal(faceNormal, vertexToFaceNormals[vb]);
            const Vector3f nc = ComputeAverageNormal(faceNormal, vertexToFaceNormals[vc]);

            AppendNewNormalIndex(na);
            AppendNewNormalIndex(nb);
            AppendNewNormalIndex(nc);
        }
    }

    int normalAttributeIndex = GetNormalAttributeIndex();
    if(normalAttributeIndex < 0)
    {
        normalAttributeIndex = static_cast<int>(attributes_.size());
        attributes_.emplace_back();
        indices_.emplace_back();
        nameToAttributeIndex_.emplace(std::make_pair("normal", normalAttributeIndex));
        builtinAttributeIndices_[std::to_underlying(BuiltinAttribute::Normal)] = normalAttributeIndex;
    }

    auto &normalAttribute = attributes_[normalAttributeIndex];
    normalAttribute.type_ = RawMeshAttributeData::Type::Float3;
    normalAttribute.name_ = "normal";
    normalAttribute.data_.resize(sizeof(Vector3f) * newNormals.size());
    std::memcpy(normalAttribute.data_.data(), newNormals.data(), normalAttribute.data_.size());

    indices_[normalAttributeIndex] = std::move(newNormalIndices);
}

void RawMesh::SplitByAttributes()
{
    uint32_t newVertexCount = 0;
    std::vector<std::vector<unsigned char>> newAttributeData(attributes_.size());
    std::map<std::vector<uint32_t>, uint32_t> newIndexMap;
    std::vector<uint32_t> newIndices;

    auto positionIndices = GetPositionIndices();
    for(uint32_t i = 0; i < positionIndices.size(); ++i)
    {
        std::vector<uint32_t> combinedIndices;
        for(uint32_t ai = 0; ai < attributes_.size(); ++ai)
        {
            combinedIndices.push_back(GetIndices(ai)[i]);
        }

        auto it = newIndexMap.find(combinedIndices);
        if(it == newIndexMap.end())
        {
            const uint32_t newIndex = newVertexCount++;
            for(uint32_t ai = 0; ai < attributes_.size(); ++i)
            {
                auto &attribute = GetAttribute(ai);
                auto srcUnit = GetAttributeBytes(attribute.GetType());
                auto srcIndex = combinedIndices[ai];
                auto src = attribute.GetUntypedData().GetData() + srcIndex * srcUnit;
                for(size_t bi = 0; bi < srcUnit; ++bi)
                {
                    newAttributeData[ai].push_back(src[bi]);
                }
            }
            it = newIndexMap.emplace(std::make_pair(std::move(combinedIndices), newIndex)).first;
        }
        newIndices.push_back(it->second);
    }

    for(uint32_t ai = 0; ai < attributes_.size(); ++ai)
    {
        attributes_[ai].data_ = std::move(newAttributeData[ai]);
        indices_[ai] = newIndices;
    }
}

void RawMesh::NormalizePositionTo(const Vector3f &targetLower, const Vector3f &targetUpper)
{
    Vector3f currentLower(FLT_MAX), currentUpper(-FLT_MAX);
    for(auto &position : GetPositionData())
    {
        currentLower = Min(currentLower, position);
        currentUpper = Max(currentUpper, position);
    }
    const Vector3f currentExtent = currentUpper - currentLower;
    const Vector3f targetExtent = targetUpper - targetLower;

    const Vector3f idealScaleFactor = targetExtent / Max(currentExtent, Vector3f(1e-6f));
    const float scale = (std::min)({ idealScaleFactor.x, idealScaleFactor.y, idealScaleFactor.z });
    const Vector3f bias = 0.5f * (targetUpper + targetLower) - scale * 0.5f * (currentLower + currentUpper);

    for(auto& position : GetPositionData())
    {
        position = scale * position + bias;
    }
}

RawMesh RawMesh::LoadWavefrontObj(const std::string& filename)
{
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig readerConfig;
    readerConfig.triangulate = true;
    readerConfig.vertex_color = true;
    if(!reader.ParseFromFile(filename, readerConfig))
    {
        throw Exception(reader.Error());
    }

    auto &attributes = reader.GetAttrib();
    RawMesh mesh;

    // Vertex attributes

    std::ranges::fill(mesh.builtinAttributeIndices_, -1);

    {
        assert(attributes.vertices.size() % 3 == 0);
        mesh.builtinAttributeIndices_[std::to_underlying(BuiltinAttribute::Position)] = mesh.attributes_.size();
        auto &positionAttribute = mesh.attributes_.emplace_back();
        positionAttribute.type_ = RawMeshAttributeData::Float3;
        positionAttribute.name_ = "position";
        positionAttribute.data_.resize(attributes.vertices.size() * sizeof(float));
        std::memcpy(positionAttribute.data_.data(), attributes.vertices.data(), positionAttribute.data_.size());
    }

    if(!attributes.normals.empty())
    {
        assert(attributes.normals.size() % 3 == 0);
        mesh.builtinAttributeIndices_[std::to_underlying(BuiltinAttribute::Normal)] = mesh.attributes_.size();
        auto& normalAttribute = mesh.attributes_.emplace_back();
        normalAttribute.type_ = RawMeshAttributeData::Float3;
        normalAttribute.name_ = "normal";
        normalAttribute.data_.resize(attributes.normals.size() * sizeof(float));
        std::memcpy(normalAttribute.data_.data(), attributes.normals.data(), normalAttribute.data_.size());

        auto normals = normalAttribute.GetData<Vector3f>();
        for(auto& normal : normals)
        {
            normal = normal == Vector3f(0) ? Vector3f(0) : Normalize(normal);
        }
    }

    if(!attributes.texcoords.empty())
    {
        assert(attributes.texcoords.size() % 2 == 0);
        mesh.builtinAttributeIndices_[std::to_underlying(BuiltinAttribute::UV)] = mesh.attributes_.size();
        auto& texCoordAttribute = mesh.attributes_.emplace_back();
        texCoordAttribute.type_ = RawMeshAttributeData::Float2;
        texCoordAttribute.name_ = "uv";
        texCoordAttribute.data_.resize(attributes.texcoords.size() * sizeof(float));
        std::memcpy(texCoordAttribute.data_.data(), attributes.texcoords.data(), texCoordAttribute.data_.size());
    }

    if(!attributes.colors.empty())
    {
        assert(attributes.colors.size() % 3 == 0);
        mesh.builtinAttributeIndices_[std::to_underlying(BuiltinAttribute::Color)] = mesh.attributes_.size();
        auto &colorAttribute = mesh.attributes_.emplace_back();
        colorAttribute.type_ = RawMeshAttributeData::Float3;
        colorAttribute.name_ = "color";
        colorAttribute.data_.resize(attributes.colors.size() * sizeof(float));
        std::memcpy(colorAttribute.data_.data(), attributes.colors.data(), colorAttribute.data_.size());
    }

    // Indices

    mesh.indices_.resize(mesh.attributes_.size());

    for(auto &shape : reader.GetShapes())
    {
        assert(shape.mesh.indices.size() % 3 == 0);

        auto &positionIndices = mesh.indices_[mesh.GetBuiltinAttributeIndex(BuiltinAttribute::Position)];
        positionIndices.reserve(positionIndices.size() + shape.mesh.indices.size());
        for(const tinyobj::index_t &index : shape.mesh.indices)
        {
            positionIndices.push_back(index.vertex_index);
        }

        if(const int ii = mesh.GetBuiltinAttributeIndex(BuiltinAttribute::Normal); ii >= 0)
        {
            auto &normalIndices = mesh.indices_[ii];
            normalIndices.reserve(normalIndices.size() + shape.mesh.indices.size());
            for(const tinyobj::index_t &index : shape.mesh.indices)
            {
                normalIndices.push_back(index.normal_index);
            }
        }

        if(const int ii = mesh.GetBuiltinAttributeIndex(BuiltinAttribute::UV); ii >= 0)
        {
            auto &uvIndices = mesh.indices_[ii];
            uvIndices.reserve(uvIndices.size() + shape.mesh.indices.size());
            for(const tinyobj::index_t &index : shape.mesh.indices)
            {
                uvIndices.push_back(index.texcoord_index);
            }
        }

        if(const int ii = mesh.GetBuiltinAttributeIndex(BuiltinAttribute::Color); ii >= 0)
        {
            auto &colorIndices = mesh.indices_[ii];
            colorIndices.reserve(colorIndices.size() + shape.mesh.indices.size());
            for(const tinyobj::index_t &index : shape.mesh.indices)
            {
                colorIndices.push_back(index.vertex_index);
            }
        }
    }
    
    mesh.triangleCount_ = mesh.indices_[0].size() / 3;

    // Name -> attribute

    for(auto&& [index, attribute] : std::views::enumerate(mesh.attributes_))
    {
        mesh.nameToAttributeIndex_[attribute.GetName()] = index;
    }

    return mesh;
}

template<std::floating_point T>
void WriteOFFFile(const std::string &filename, Span<Vector3<T>> positions, Span<uint32_t> indices)
{
    std::ofstream fout(filename, std::ofstream::trunc);
    if(!fout)
    {
        throw Exception("WriteOFFFile: fail to open " + filename);
    }

    fout << "OFF\n";

    if(indices.IsEmpty())
    {
        fout << positions.size() << " " << (positions.size() / 3) << " 0\n";
    }
    else
    {
        fout << positions.size() << " " << (indices.size() / 3) << " 0\n";
    }

    for(auto &p : positions)
    {
        fout << ToString(p.x) << " " << ToString(p.y) << " " << ToString(p.z) << "\n";
    }

    if(indices.IsEmpty())
    {
        for(uint32_t i = 0; i < positions.size(); i += 3)
        {
            fout << "3 " << i << " " << (i + 1) << " " << (i + 2) << "\n";
        }
    }
    else
    {
        for(uint32_t i = 0; i < indices.size(); i += 3)
        {
            fout << "3 " << indices[i] << " " << indices[i + 1] << " " << indices[i + 2] << "\n";
        }
    }
}

template void WriteOFFFile(const std::string &, Span<Vector3<float>>, Span<uint32_t>);
template void WriteOFFFile(const std::string &, Span<Vector3<double>>, Span<uint32_t>);

RTRC_GEO_END
