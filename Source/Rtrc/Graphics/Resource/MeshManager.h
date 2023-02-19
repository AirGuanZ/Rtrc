#pragma once

#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Utility/ObjectCache.h>

RTRC_BEGIN

class Device;

class MeshManager : public Uncopyable
{
public:

    struct Options
    {
        bool generateTangentIfNotPresent = false;
        bool noIndexBuffer               = false;

        auto operator<=>(const Options &) const = default;
        size_t Hash() const;
    };

    static Mesh Load(Device *device, const std::string &filename, const Options &options);

    MeshManager();

    void SetDevice(Device *device);
    
    RC<Mesh> GetMesh(std::string_view name, const Options &options = {});

private:

    static Vector3f ComputeTangent(
        const Vector3f &B_A, const Vector3f &C_A,
        const Vector2f &b_a, const Vector2f &c_a,
        const Vector3f &nor);

    Device *device_;
    
    ObjectCache<std::pair<std::string, Options>, Mesh, true, true> meshCache_;
};

RTRC_END
