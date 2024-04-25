#pragma once

#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/StringPool.h>
#include <Rtrc/Geometry/FlatHalfEdgeMesh.h>

RTRC_BEGIN

namespace RawMeshDetail
{

    enum class AttributeType
    {
        Float2,
        Float3,
    };

    template<typename> struct AttributeTypeToDataType { };
    template<> struct AttributeTypeToDataType<Vector2f> { static constexpr AttributeType Value = AttributeType::Float2; };
    template<> struct AttributeTypeToDataType<Vector3f> { static constexpr AttributeType Value = AttributeType::Float3; };
    
} // namespace RawMeshDetail

class RawMeshAttributeData
{
public:

    using Type = RawMeshDetail::AttributeType;
    using enum RawMeshDetail::AttributeType;

    Type GetType() const;
    const std::string &GetName() const;

    Span<unsigned char> GetUntypedData() const;
    MutSpan<unsigned char> GetUntypedData();

    template<typename T>
    Span<T> GetData() const;
    template<typename T>
    MutSpan<T> GetData();

private:

    friend class RawMesh;

    Type type_ = Float3;
    std::string name_;
    std::vector<unsigned char> data_;
};

// Represents a raw mesh loaded from a model file. This class stores attributes in compact arrays.
// Each polygon's vertices are associated with separate indices for their respective attributes.
class RawMesh
{
public:

    static RawMesh Load(const std::string &filename);

    // Get data

    uint32_t GetAttributeCount() const;
    uint32_t GetTriangleCount() const;

    int GetAttributeIndex(std::string_view name) const;
    bool HasAttribute(std::string_view name) const;

    Span<RawMeshAttributeData> GetAttributes() const;
    const RawMeshAttributeData &GetAttribute(uint32_t attributeIndex) const;

    const RawMeshAttributeData *GetPositions() const;
    const RawMeshAttributeData *GetNormals() const;
    const RawMeshAttributeData *GetUVs() const;

    Span<uint32_t> GetIndices(uint32_t attributeIndex) const;

private:

    enum class BuiltinAttribute
    {
        Position,
        Normal,
        UV,
        Count
    };

    static std::string_view GetExtension(const std::string &filename);

    static RawMesh LoadWavefrontObj(const std::string &filename);

    int GetBuiltinAttributeIndex(BuiltinAttribute attribute) const;

    uint32_t triangleCount_ = 0;
    std::vector<RawMeshAttributeData> attributes_;
    std::vector<std::vector<uint32_t>> indices_;
    std::array<int, EnumCount<BuiltinAttribute>> builtinAttributeIndices_ = {};
    std::map<std::string, int, std::less<>> nameToAttributeIndex_;
};

inline RawMeshAttributeData::Type RawMeshAttributeData::GetType() const
{
    return type_;
}

inline const std::string& RawMeshAttributeData::GetName() const
{
    return name_;
}

inline Span<unsigned char> RawMeshAttributeData::GetUntypedData() const
{
    return data_;
}

inline MutSpan<unsigned char> RawMeshAttributeData::GetUntypedData()
{
    return data_;
}

template <typename T>
Span<T> RawMeshAttributeData::GetData() const
{
    assert(RawMeshDetail::AttributeTypeToDataType<T>::Value == type_);
    const size_t count = data_.size() / sizeof(T);
    return Span<T>(reinterpret_cast<const T*>(data_.data()), count);
}

template <typename T>
MutSpan<T> RawMeshAttributeData::GetData()
{
    assert(RawMeshDetail::AttributeTypeToDataType<T>::Value == type_);
    const size_t count = data_.size() / sizeof(T);
    return MutSpan<T>(reinterpret_cast<T *>(data_.data()), count);
}

inline uint32_t RawMesh::GetAttributeCount() const
{
    return attributes_.size();
}

inline uint32_t RawMesh::GetTriangleCount() const
{
    return triangleCount_;
}

inline int RawMesh::GetAttributeIndex(std::string_view name) const
{
    const auto it = nameToAttributeIndex_.find(name);
    return it != nameToAttributeIndex_.end() ? it->second : -1;
}

inline bool RawMesh::HasAttribute(std::string_view name) const
{
    return nameToAttributeIndex_.contains(name);
}

inline Span<RawMeshAttributeData> RawMesh::GetAttributes() const
{
    return attributes_;
}

inline const RawMeshAttributeData& RawMesh::GetAttribute(uint32_t attributeIndex) const
{
    return attributes_[attributeIndex];
}

inline const RawMeshAttributeData* RawMesh::GetPositions() const
{
    const int index = GetBuiltinAttributeIndex(BuiltinAttribute::Position);
    return index >= 0 ? &attributes_[index] : nullptr;
}

inline const RawMeshAttributeData* RawMesh::GetNormals() const
{
    const int index = GetBuiltinAttributeIndex(BuiltinAttribute::Normal);
    return index >= 0 ? &attributes_[index] : nullptr;
}

inline const RawMeshAttributeData* RawMesh::GetUVs() const
{
    const int index = GetBuiltinAttributeIndex(BuiltinAttribute::UV);
    return index >= 0 ? &attributes_[index] : nullptr;
}

inline Span<uint32_t> RawMesh::GetIndices(uint32_t attributeIndex) const
{
    return indices_[attributeIndex];
}

inline int RawMesh::GetBuiltinAttributeIndex(BuiltinAttribute attribute) const
{
    return builtinAttributeIndices_[std::to_underlying(attribute)];
}

RTRC_END
