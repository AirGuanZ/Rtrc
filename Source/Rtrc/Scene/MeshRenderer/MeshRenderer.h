#pragma once

#include <Rtrc/Resource/Material/MaterialInstance.h>
#include <Rtrc/Resource/Mesh/Mesh.h>
#include <Rtrc/Scene/SceneObject.h>
#include <Core/EnumFlags.h>

RTRC_BEGIN

class MeshRendererManager;

enum class MeshRendererFlagBit : uint32_t
{
    None       = 0,
    OpaqueTlas = 1 << 0,
};
RTRC_DEFINE_ENUM_FLAGS(MeshRendererFlagBit)

class MeshRenderer : public SceneObject
{
public:

    using Flags = EnumFlagsMeshRendererFlagBit;

    ~MeshRenderer() override;

    RTRC_SET_GET(Flags,                Flags,    flags_)
    RTRC_SET_GET(RC<Mesh>,             Mesh,     mesh_)
    RTRC_SET_GET(RC<MaterialInstance>, Material, material_)

private:

    friend class MeshRendererManager;

    MeshRenderer() = default;

    MeshRendererManager                *manager_ = nullptr;
    std::list<MeshRenderer *>::iterator iterator_;

    Flags                flags_ = MeshRendererFlagBit::None;
    RC<Mesh>             mesh_;
    RC<MaterialInstance> material_;
};

RTRC_END
