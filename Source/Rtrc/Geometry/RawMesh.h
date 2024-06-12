#pragma once

#include <Rtrc/Core/Math/AABB.h>
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

    constexpr size_t GetAttributeBytes(AttributeType type)
    {
        constexpr std::array<size_t, 2> ret =
        {
            8,
            12
        };
        return ret[std::to_underlying(type)];
    }

    template<typename>
    struct AttributeTypeToDataType
    {
        
    };
    template<>
    struct AttributeTypeToDataType<Vector2f>
    {
        static constexpr AttributeType Value = AttributeType::Float2;
    };
    template<>
    struct AttributeTypeToDataType<Vector3f>
    {
        static constexpr AttributeType Value = AttributeType::Float3;
    };
    
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

// Represents a raw triangle mesh loaded from a model file. This class stores attributes in compact arrays.
// Each triangle's vertices are associated with separate indices for their respective attributes.
class RawMesh
{
public:

    // ======================== Builder ========================

    static RawMesh Load(const std::string &filename);

    void SetTriangleCount(uint32_t count);
    void AddAttribute(
        std::string                name,
        RawMeshAttributeData::Type type,
        std::vector<unsigned char> data,
        std::vector<uint32_t>      indices);

    // ======================== Get data ========================

    uint32_t GetAttributeCount() const;
    uint32_t GetTriangleCount() const;

    int GetAttributeIndex(std::string_view name) const;
    bool HasAttribute(std::string_view name) const;

    Span<RawMeshAttributeData> GetAttributes() const;
    const RawMeshAttributeData &GetAttribute(uint32_t attributeIndex) const;

    int GetPositionAttributeIndex() const;
    int GetNormalAttributeIndex() const;
    int GetUVAttributeIndex() const;

    bool HasNormals() const;
    bool HasUVs() const;

    RawMeshAttributeData *GetPositionAttribute();
    RawMeshAttributeData *GetNormalAttribute();
    RawMeshAttributeData *GetUVAttribute();

    const RawMeshAttributeData *GetPositionAttribute() const;
    const RawMeshAttributeData *GetNormalAttribute() const;
    const RawMeshAttributeData *GetUVAttribute() const;

    Span<Vector3f> GetPositionData() const;
    Span<Vector3f> GetNormalData() const;
    Span<Vector2f> GetUVData() const;

    MutSpan<Vector3f> GetPositionData();
    MutSpan<Vector3f> GetNormalData();
    MutSpan<Vector2f> GetUVData();

    Span<uint32_t> GetIndices(uint32_t attributeIndex) const;

    Span<uint32_t> GetPositionIndices() const;
    Span<uint32_t> GetNormalIndices() const;
    Span<uint32_t> GetUVIndices() const;

    FlatHalfEdgeMesh CreateFlatHalfEdgeMesh() const;

    AABB3f ComputeBoundingBox() const;

    // ======================== In-place modifier ========================

    // Set angle threshold to 1 to use face normals
    void RecalculateNormal(float cosAngleThreshold = 1);

    // Split vertices by normal, UV, etc., so that the position index can be used to reference all available attributes.
    // Suitable for rendering.
    void SplitByAttributes();

    // Scale and translate the mesh to fit into the given bounding box
    void NormalizePositionTo(const Vector3f &targetLower, const Vector3f &targetUpper);

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

inline int RawMesh::GetPositionAttributeIndex() const
{
    return GetBuiltinAttributeIndex(BuiltinAttribute::Position);
}

inline int RawMesh::GetNormalAttributeIndex() const
{
    return GetBuiltinAttributeIndex(BuiltinAttribute::Normal);
}

inline int RawMesh::GetUVAttributeIndex() const
{
    return GetBuiltinAttributeIndex(BuiltinAttribute::UV);
}

inline bool RawMesh::HasNormals() const
{
    return GetBuiltinAttributeIndex(BuiltinAttribute::Normal) >= 0;
}

inline bool RawMesh::HasUVs() const
{
    return GetBuiltinAttributeIndex(BuiltinAttribute::UV) >= 0;
}

inline RawMeshAttributeData *RawMesh::GetPositionAttribute()
{
    const int index = GetBuiltinAttributeIndex(BuiltinAttribute::Position);
    return index >= 0 ? &attributes_[index] : nullptr;
}

inline RawMeshAttributeData *RawMesh::GetNormalAttribute()
{
    const int index = GetBuiltinAttributeIndex(BuiltinAttribute::Normal);
    return index >= 0 ? &attributes_[index] : nullptr;
}

inline RawMeshAttributeData *RawMesh::GetUVAttribute()
{
    const int index = GetBuiltinAttributeIndex(BuiltinAttribute::UV);
    return index >= 0 ? &attributes_[index] : nullptr;
}

inline const RawMeshAttributeData* RawMesh::GetPositionAttribute() const
{
    const int index = GetBuiltinAttributeIndex(BuiltinAttribute::Position);
    return index >= 0 ? &attributes_[index] : nullptr;
}

inline const RawMeshAttributeData* RawMesh::GetNormalAttribute() const
{
    const int index = GetBuiltinAttributeIndex(BuiltinAttribute::Normal);
    return index >= 0 ? &attributes_[index] : nullptr;
}

inline const RawMeshAttributeData* RawMesh::GetUVAttribute() const
{
    const int index = GetBuiltinAttributeIndex(BuiltinAttribute::UV);
    return index >= 0 ? &attributes_[index] : nullptr;
}

inline Span<Vector3f> RawMesh::GetPositionData() const
{
    return GetPositionAttribute()->GetData<Vector3f>();
}

inline Span<Vector3f> RawMesh::GetNormalData() const
{
    if(auto attrib = GetNormalAttribute())
    {
        return attrib->GetData<Vector3f>();
    }
    return {};
}

inline Span<Vector2f> RawMesh::GetUVData() const
{
    if(auto attrib = GetUVAttribute())
    {
        return attrib->GetData<Vector2f>();
    }
    return {};
}

inline MutSpan<Vector3f> RawMesh::GetPositionData()
{
    return GetPositionAttribute()->GetData<Vector3f>();
}

inline MutSpan<Vector3f> RawMesh::GetNormalData()
{
    if(auto attrib = GetNormalAttribute())
    {
        return attrib->GetData<Vector3f>();
    }
    return {};
}

inline MutSpan<Vector2f> RawMesh::GetUVData()
{
    if(auto attrib = GetUVAttribute())
    {
        return attrib->GetData<Vector2f>();
    }
    return {};
}

inline Span<uint32_t> RawMesh::GetIndices(uint32_t attributeIndex) const
{
    return indices_[attributeIndex];
}

inline Span<uint32_t> RawMesh::GetPositionIndices() const
{
    const int index = GetPositionAttributeIndex();
    assert(index >= 0);
    return GetIndices(index);
}

inline Span<uint32_t> RawMesh::GetNormalIndices() const
{
    const int index = GetNormalAttributeIndex();
    assert(index >= 0);
    return GetIndices(index);
}

inline Span<uint32_t> RawMesh::GetUVIndices() const
{
    const int index = GetUVAttributeIndex();
    assert(index >= 0);
    return GetIndices(index);
}

inline FlatHalfEdgeMesh RawMesh::CreateFlatHalfEdgeMesh() const
{
    return FlatHalfEdgeMesh::Build(GetPositionIndices());
}

inline AABB3f RawMesh::ComputeBoundingBox() const
{
    return std::ranges::fold_left(
        GetPositionData(), AABB3f{}, [](const auto &lhs, const auto &rhs) { return lhs | rhs; });
}

inline int RawMesh::GetBuiltinAttributeIndex(BuiltinAttribute attribute) const
{
    return builtinAttributeIndices_[std::to_underlying(attribute)];
}

RTRC_END
