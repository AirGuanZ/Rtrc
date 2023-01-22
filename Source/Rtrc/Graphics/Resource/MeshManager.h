#pragma once

#include <filesystem>

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
        bool noIndexBuffer = false;

        auto operator<=>(const Options &) const = default;

        size_t Hash() const;
    };

    static Mesh Load(Device *device, const std::string &filename, const Options &options);

    MeshManager();

    void SetDevice(Device *device);

    void AddFiles(const std::set<std::filesystem::path> &filenames);

    RC<Mesh> GetMesh(const std::string &name, const Options &options = {});

private:

    static Vector3f ComputeTangent(
        const Vector3f &B_A, const Vector3f &C_A,
        const Vector2f &b_a, const Vector2f &c_a,
        const Vector3f &nor);

    Device *device_;

    std::map<std::string, std::string, std::less<>> meshNameToFilename_;
    ObjectCache<std::pair<std::string, Options>, Mesh, true, true> meshCache_;
};

bool operator<(
    const std::pair<std::string, MeshManager::Options> &lhs,
    const std::pair<std::string_view, MeshManager::Options> &rhs);

bool operator<(
    const std::pair<std::string_view, MeshManager::Options> &lhs,
    const std::pair<std::string, MeshManager::Options> &rhs);

RTRC_END
