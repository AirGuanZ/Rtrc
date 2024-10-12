#pragma once

#include <Rtrc/Core/Math/AABB.h>
#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/StringPool.h>
#include <Rtrc/Geometry/HalfedgeMesh.h>

RTRC_GEO_BEGIN

namespace RawMeshDetail
{

    enum class AttributeType
    {
        Float,
        Float2,
        Float3,
    };

    constexpr size_t GetAttributeBytes(AttributeType type)
    {
        constexpr std::array<size_t, 3> ret =
        {
            4,
            8,
            12
        };
        return ret[std::to_underlying(type)];
    }

    template<typename>
    struct AttributeTypeToDataType;

#define ADD_TYPE_MAP(SRC, DST)                                     \
    template<>                                                     \
    struct AttributeTypeToDataType<SRC>                            \
    {                                                              \
        static constexpr AttributeType Value = AttributeType::DST; \
    }

    ADD_TYPE_MAP(float,    Float);
    ADD_TYPE_MAP(Vector2f, Float2);
    ADD_TYPE_MAP(Vector3f, Float3);

#undef ADD_TYPE_MAP
    
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
    int GetColorAttributeIndex() const;

    bool HasNormals() const;
    bool HasUVs() const;
    bool HasColors() const;

    RawMeshAttributeData *GetPositionAttribute();
    RawMeshAttributeData *GetNormalAttribute();
    RawMeshAttributeData *GetUVAttribute();
    RawMeshAttributeData *GetColorAttribute();

    const RawMeshAttributeData *GetPositionAttribute() const;
    const RawMeshAttributeData *GetNormalAttribute() const;
    const RawMeshAttributeData *GetUVAttribute() const;
    const RawMeshAttributeData *GetColorAttribute() const;

    Span<Vector3f> GetPositionData() const;
    Span<Vector3f> GetNormalData() const;
    Span<Vector2f> GetUVData() const;
    Span<Vector3f> GetColorData() const;

    MutSpan<Vector3f> GetPositionData();
    MutSpan<Vector3f> GetNormalData();
    MutSpan<Vector2f> GetUVData();
    MutSpan<Vector3f> GetColorData();

    std::vector<Vector3d> GetPositionDataDouble() const;

    Span<uint32_t> GetIndices(uint32_t attributeIndex) const;

    Span<uint32_t> GetPositionIndices() const;
    Span<uint32_t> GetNormalIndices() const;
    Span<uint32_t> GetUVIndices() const;
    Span<uint32_t> GetColorIndices() const;

    HalfedgeMesh CreateHalfEdgeMesh() const;

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
        Color,
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

// If empty, indices are implicitly defined as [0, 1, 2, ...]
template<std::floating_point T>
void WriteOFFFile(const std::string &filename, Span<Vector3<T>> positions, Span<uint32_t> indices);

// 'positions' is required. 'normals' and 'uvs' are optional.
// 'normal' and 'uv' should be in per-wedge style, if provided.
// All 'xxxIndices' fields are optional. If left empty, they are implicitly defined as [0, 1, 2, ...]
template<std::floating_point T>
void WriteOBJFile(
    const std::string &filename,
    Span<Vector3<T>>   positions,
    Span<uint32_t>     positionIndices,
    Span<Vector3<T>>   normals,
    Span<uint32_t>     normalIndices,
    Span<Vector2<T>>   uvs,
    Span<uint32_t>     uvIndices);

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

inline int RawMesh::GetColorAttributeIndex() const
{
    return GetBuiltinAttributeIndex(BuiltinAttribute::Color);
}

inline bool RawMesh::HasNormals() const
{
    return GetBuiltinAttributeIndex(BuiltinAttribute::Normal) >= 0;
}

inline bool RawMesh::HasUVs() const
{
    return GetBuiltinAttributeIndex(BuiltinAttribute::UV) >= 0;
}

inline bool RawMesh::HasColors() const
{
    return GetBuiltinAttributeIndex(BuiltinAttribute::Color) >= 0;
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

inline RawMeshAttributeData *RawMesh::GetColorAttribute()
{
    const int index = GetBuiltinAttributeIndex(BuiltinAttribute::Color);
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

inline const RawMeshAttributeData *RawMesh::GetColorAttribute() const
{
    const int index = GetBuiltinAttributeIndex(BuiltinAttribute::Color);
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

inline Span<Vector3f> RawMesh::GetColorData() const
{
    if(auto attrib = GetColorAttribute())
    {
        return attrib->GetData<Vector3f>();
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

inline MutSpan<Vector3f> RawMesh::GetColorData()
{
    if(auto attrib = GetColorAttribute())
    {
        return attrib->GetData<Vector3f>();
    }
    return {};
}

inline std::vector<Vector3d> RawMesh::GetPositionDataDouble() const
{
    auto positions = GetPositionData();
    std::vector<Vector3d> result(positions.size());
    for(size_t i = 0; i < positions.size(); ++i)
    {
        result[i] = positions[i].To<double>();
    }
    return result;
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

inline Span<uint32_t> RawMesh::GetColorIndices() const
{
    const int index = GetColorAttributeIndex();
    assert(index >= 0);
    return GetIndices(index);
}

inline HalfedgeMesh RawMesh::CreateHalfEdgeMesh() const
{
    return HalfedgeMesh::Build(GetPositionIndices());
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

RTRC_GEO_END
