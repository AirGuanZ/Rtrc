#pragma once

#include <Rtrc/Graphics/Material/Material.h>
#include <Rtrc/Graphics/Device/Device.h>

RTRC_BEGIN

enum class BuiltinShader
{
    DeferredLighting,
    Count
};

enum class BuiltinTexture
{
    Black2D, // R8G8B8A8_UNorm, RGB = 0, A = 1
    White2D, // R8G8B8A8_UNorm, RGB = 1, A = 1
    Count
};

enum class BuiltinMesh
{
    Cube,
    Count
};

class BuiltinResourceManager : public Uncopyable
{
public:

    explicit BuiltinResourceManager(Device &device);

    const RC<Shader> &GetBuiltinShader(BuiltinShader shader) const;

    const RC<Mesh> &GetBuiltinMesh(BuiltinMesh mesh) const;

    const RC<Texture> &GetBuiltinTexture(BuiltinTexture texture) const;

private:

    void LoadBuiltinMaterials();

    void LoadBuiltinTextures();

    void LoadBuiltinMeshes();

    Device &device_;

    MaterialManager materialManager_;
    std::array<RC<Shader>,  EnumCount<BuiltinShader>>  shaders_;
    std::array<RC<Texture>, EnumCount<BuiltinTexture>> textures_;
    std::array<RC<Mesh>,    EnumCount<BuiltinMesh>>    meshes_;
};

RTRC_END
