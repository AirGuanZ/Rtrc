#include <tiny_obj_loader.h>

#include <Rtrc/Core/Enumerate.h>
#include <Rtrc/Core/String.h>
#include <Rtrc/Geometry/RawMesh.h>

RTRC_BEGIN

RawMesh RawMesh::Load(const std::string& filename)
{
    const std::string_view ext = GetExtension(filename);
    if(ToLower(ext) == ".obj")
    {
        return LoadWavefrontObj(filename);
    }
    throw Exception(fmt::format("Unsupported mesh file extension: '{}'", ext));
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
            normal = normal == Vector3f(0) ? Normalize(normal) : Vector3f();
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
    }
    
    mesh.triangleCount_ = mesh.indices_[0].size() / 3;

    // Name -> attribute

    for(auto&& [index, attribute] : Enumerate(mesh.attributes_))
    {
        mesh.nameToAttributeIndex_[attribute.GetName()] = index;
    }

    return mesh;
}

RTRC_END
