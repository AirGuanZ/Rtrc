#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Graphics/Resource/MeshManager.h>
#include <Rtrc/Math/LowDiscrepancy.h>
#include <Rtrc/Renderer/Utility/FullscreenPrimitive.h>
#include <Rtrc/Utility/Filesystem/DirectoryFilter.h>

RTRC_BEGIN

BuiltinResourceManager::BuiltinResourceManager(ObserverPtr<Device> device)
    : device_(device)
{
    materialManager_.SetDebugMode(RTRC_DEBUG);
    materialManager_.SetDevice(device_);
    materialManager_.AddFiles($rtrc_get_files("Asset/Builtin/Material/*/*.*"));
    materialManager_.AddIncludeDirectory("Asset/Builtin/Material");

    LoadBuiltinTextures();
    LoadBuiltinMeshes();
    LoadBuiltinMaterials();

    fullscreenTriangle_ = GetFullscreenTriangle(*device_);
    fullscreenQuad_ = GetFullscreenQuad(*device_);
}

ObserverPtr<Device> BuiltinResourceManager::GetDevice() const
{
    return device_;
}

const RC<Mesh> &BuiltinResourceManager::GetBuiltinMesh(BuiltinMesh mesh) const
{
    return meshes_[std::to_underlying(mesh)];
}

const RC<Texture> &BuiltinResourceManager::GetBuiltinTexture(BuiltinTexture texture) const
{
    return textures_[std::to_underlying(texture)];
}

const RC<Material> &BuiltinResourceManager::GetBuiltinMaterial(BuiltinMaterial material) const
{
    return materials_[std::to_underlying(material)];
}

const Mesh &BuiltinResourceManager::GetFullscreenTriangleMesh() const
{
    return fullscreenTriangle_;
}

const Mesh &BuiltinResourceManager::GetFullscreenQuadMesh() const
{
    return fullscreenQuad_;
}

const Image<uint8_t> &BuiltinResourceManager::GetBlueNoise256() const
{
    return blueNoise256_;
}

void BuiltinResourceManager::LoadBuiltinTextures()
{
    textures_[std::to_underlying(BuiltinTexture::Black2D)] =
        device_->CreateColorTexture2D(0, 0, 0, 255, "BuiltinBlack2D");
    textures_[std::to_underlying(BuiltinTexture::White2D)] =
        device_->CreateColorTexture2D(255, 255, 255, 255, "BuiltinWhite2D");

    blueNoise256_ = Image<uint8_t>::Load("Asset/Builtin/Texture/BlueNoise256.png");
    textures_[std::to_underlying(BuiltinTexture::BlueNoise256)] =
        device_->CreateAndUploadTexture2D(
            RHI::TextureDesc
            {
                .dim                  = RHI::TextureDimension::Tex2D,
                .format               = RHI::Format::R8_UNorm,
                .width                = blueNoise256_.GetWidth(),
                .height               = blueNoise256_.GetHeight(),
                .depth                = 1,
                .mipLevels            = 1,
                .sampleCount          = 1,
                .usage                = RHI::TextureUsage::ShaderResource,
                .initialLayout        = RHI::TextureLayout::Undefined,
                .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
            }, blueNoise256_.GetData(), RHI::TextureLayout::ShaderTexture);
}

void BuiltinResourceManager::LoadBuiltinMeshes()
{
    MeshManager::Flags flags = MeshManagerDetail::LoadMeshFlagBit::GenerateTangentIfNotPresent;
    if(device_->IsRayTracingEnabled())
    {
        flags |= MeshManagerDetail::LoadMeshFlagBit::AllowBlas;
    }

#define LOAD_BUILTIN_MESH(NAME) \
    meshes_[std::to_underlying(BuiltinMesh::NAME)] = \
        ToRC(MeshManager::Load(device_, "Asset/Builtin/Mesh/" #NAME ".obj", flags))

    LOAD_BUILTIN_MESH(Cube);

#undef LOAD_BUILTIN_MESH
}

void BuiltinResourceManager::LoadBuiltinMaterials()
{
#define LOAD_BUILTIN_MATERIAL(NAME) \
    materials_[std::to_underlying(BuiltinMaterial::NAME)] = materialManager_.GetMaterial("Builtin/" #NAME)
    
    LOAD_BUILTIN_MATERIAL(DeferredLighting);
    LOAD_BUILTIN_MATERIAL(Atmosphere);
    LOAD_BUILTIN_MATERIAL(ShadowMask);
    LOAD_BUILTIN_MATERIAL(PathTracing);

#undef LOAD_BUILTIN_MATERIAL
}

RTRC_END
