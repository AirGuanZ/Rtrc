#pragma once

#include <Rtrc/Graphics/Material/MaterialInstance.h>
#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Renderer/Scene/SceneObject.h>

RTRC_BEGIN

rtrc_struct(StaticMeshCBuffer)
{
    rtrc_var(Matrix4x4f, localToWorld);
    rtrc_var(Matrix4x4f, worldToLocal);
    rtrc_var(Matrix4x4f, localToCamera);
    rtrc_var(Matrix4x4f, localToClip);
};

class StaticMesh : public SceneObject
{
public:

    StaticMesh() = default;
    StaticMesh(StaticMesh &&other) noexcept;
    StaticMesh &operator=(StaticMesh &&other) noexcept;

    void Swap(StaticMesh &other) noexcept;

    void SetMesh(RC<Mesh> mesh);
    void SetMaterial(RC<MaterialInstance> matInst);

    const RC<Mesh> &GetMesh() const;
    const RC<MaterialInstance> &GetMaterial() const;

private:

    RC<Mesh> mesh_;
    RC<MaterialInstance> matInst_;
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

inline const RC<Mesh> &StaticMesh::GetMesh() const
{
    return mesh_;
}

inline const RC<MaterialInstance> &StaticMesh::GetMaterial() const
{
    return matInst_;
}

RTRC_END
