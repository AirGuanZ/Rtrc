#pragma once

#include <Rtrc/Graphics/Material/MaterialInstance.h>
#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Renderer/Scene/SceneObject.h>

RTRC_BEGIN

rtrc_struct(StaticMeshCBuffer)
{
    rtrc_var(float4x4, localToWorld);
    rtrc_var(float4x4, worldToLocal);
    rtrc_var(float4x4, localToCamera);
    rtrc_var(float4x4, localToClip);
};

class StaticMesh : public SceneObject
{
public:
    
    using PushConstantData = StaticVector<unsigned char, RHI::STANDARD_PUSH_CONSTANT_BLOCK_SIZE>;

    StaticMesh() = default;
    StaticMesh(StaticMesh &&other) noexcept;
    StaticMesh &operator=(StaticMesh &&other) noexcept;

    void Swap(StaticMesh &other) noexcept;

    void SetMesh(RC<Mesh> mesh);
    void SetMaterial(RC<MaterialInstance> matInst);

    template<typename T>
    void SetPushConstantData(const T &value);
    void SetPushConstantData(const void *data, uint32_t bytes);

    const RC<Mesh> &GetMesh() const;
    const RC<MaterialInstance> &GetMaterial() const;
    Span<unsigned char> GetPushConstantData() const;

private:

    RC<Mesh> mesh_;
    RC<MaterialInstance> matInst_;
    Box<PushConstantData> pushConstantData_;
};

inline StaticMesh::StaticMesh(StaticMesh &&other) noexcept
{
    Swap(other);
}

inline StaticMesh &StaticMesh::operator=(StaticMesh &&other) noexcept
{
    Swap(other);
    return *this;
}

inline void StaticMesh::SetMesh(RC<Mesh> mesh)
{
    mesh_ = std::move(mesh);
}

inline void StaticMesh::SetMaterial(RC<MaterialInstance> matInst)
{
    matInst_ = std::move(matInst);
}

template<typename T>
void StaticMesh::SetPushConstantData(const T &value)
{
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) <= RHI::STANDARD_PUSH_CONSTANT_BLOCK_SIZE);
    this->SetPushConstantData(&value, sizeof(value));
}

inline void StaticMesh::SetPushConstantData(const void *data, uint32_t bytes)
{
    if(!pushConstantData_)
    {
        pushConstantData_ = MakeBox<StaticVector<unsigned char, RHI::STANDARD_PUSH_CONSTANT_BLOCK_SIZE>>();
    }
    assert(bytes <= RHI::STANDARD_PUSH_CONSTANT_BLOCK_SIZE);
    pushConstantData_->Resize(bytes);
    std::memcpy(pushConstantData_->GetData(), data, bytes);
}

inline const RC<Mesh> &StaticMesh::GetMesh() const
{
    return mesh_;
}

inline const RC<MaterialInstance> &StaticMesh::GetMaterial() const
{
    return matInst_;
}

inline Span<unsigned char> StaticMesh::GetPushConstantData() const
{
    return *pushConstantData_;
}

RTRC_END
