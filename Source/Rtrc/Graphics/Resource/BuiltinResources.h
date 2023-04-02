#pragma once

#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Resource/MaterialManager.h>
#include <Rtrc/Utility/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

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

enum class BuiltinMaterial
{
    DeferredLighting,
    DeferredLighting2,
    Atmosphere,
    Count
};

class BuiltinResourceManager : public Uncopyable
{
public:

    explicit BuiltinResourceManager(ObserverPtr<Device> device);

    ObserverPtr<Device> GetDevice() const;
    
    const RC<Mesh>     &GetBuiltinMesh    (BuiltinMesh     mesh)     const;
    const RC<Texture>  &GetBuiltinTexture (BuiltinTexture  texture)  const;
    const RC<Material> &GetBuiltinMaterial(BuiltinMaterial material) const;

    const Mesh &GetFullscreenTriangleMesh() const;
    const Mesh &GetFullscreenQuadMesh()     const;

private:

    void LoadBuiltinTextures();
    void LoadBuiltinMeshes();
    void LoadBuiltinMaterials();

    ObserverPtr<Device> device_;
    MaterialManager materialManager_;

    std::array<RC<Texture>,  EnumCount<BuiltinTexture>>  textures_;
    std::array<RC<Mesh>,     EnumCount<BuiltinMesh>>     meshes_;
    std::array<RC<Material>, EnumCount<BuiltinMaterial>> materials_;

    Mesh fullscreenTriangle_;
    Mesh fullscreenQuad_;
};

RTRC_END
