#pragma once

#include <Core/Container/ObjectCache.h>
#include <Rtrc/Resource/Mesh/Mesh.h>

RTRC_BEGIN

class Device;

namespace MeshManagerDetail
{

    enum class LoadMeshFlagBit : uint32_t
    {
        None                        = 0,
        GenerateTangentIfNotPresent = 1 << 0,
        RemoveIndexBuffer           = 1 << 1,
        AllowBlas                   = 1 << 2,
    };
    RTRC_DEFINE_ENUM_FLAGS(LoadMeshFlagBit)
    using LoadMeshFlags = EnumFlagsLoadMeshFlagBit;

} // namespace MeshManagerDetail

class MeshManager : public Uncopyable
{
public:
    
    using Flags = MeshManagerDetail::LoadMeshFlags;

    static Mesh Load(Device *device, const std::string &filename, Flags flags = Flags::None);

    MeshManager();

    void SetDevice(Device *device);
    
    RC<Mesh> GetMesh(std::string_view name, Flags flags = Flags::None);

private:

    static Vector3f ComputeTangent(
        const Vector3f &B_A, const Vector3f &C_A,
        const Vector2f &b_a, const Vector2f &c_a,
        const Vector3f &nor);

    Device *device_;
    
    ObjectCache<std::pair<std::string, Flags>, Mesh, true, true> meshCache_;
};

RTRC_END
