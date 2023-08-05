#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Resource/ResourceManager.h>
#include <Rtrc/Utility/Filesystem/DirectoryFilter.h>

RTRC_BEGIN

ResourceManager::ResourceManager(ObserverPtr<Device> device, bool debugMode)
    : device_(device)
{
    materialManager_.SetDevice(device);
    materialManager_.SetDebugMode(debugMode);
    meshManager_.SetDevice(device);

    LoadBuiltinMeshes();
    LoadBuiltinTextures();
    LoadBuiltinMaterials();
}

void ResourceManager::AddMaterialFiles(const std::set<std::filesystem::path> &filenames)
{
    materialManager_.AddFiles(filenames);
}

void ResourceManager::AddShaderIncludeDirectory(std::string_view dir)
{
    materialManager_.AddIncludeDirectory(dir);
}

RC<Material> ResourceManager::GetMaterial(const std::string &name)
{
    return materialManager_.GetMaterial(name);
}

RC<ShaderTemplate> ResourceManager::GetShaderTemplate(const std::string &name)
{
    return materialManager_.GetShaderTemplate(name);
}

RC<Shader> ResourceManager::GetShader(const std::string &name)
{
    return materialManager_.GetShader(name);
}

RC<Mesh> ResourceManager::GetMesh(std::string_view name, MeshFlags flags)
{
    return meshManager_.GetMesh(name, flags);
}

RC<MaterialInstance> ResourceManager::CreateMaterialInstance(const std::string &name)
{
    return materialManager_.CreateMaterialInstance(name);
}

void ResourceManager::LoadBuiltinMeshes()
{
    MeshManager::Flags flags = MeshManagerDetail::LoadMeshFlagBit::GenerateTangentIfNotPresent;
    if(device_->IsRayTracingEnabled())
    {
        flags |= MeshManagerDetail::LoadMeshFlagBit::AllowBlas;
    }

#define LOAD_BUILTIN_MESH(NAME)                                     \
    ([&]                                                            \
    {                                                               \
        auto filename = "Asset/Builtin/Mesh/" #NAME ".obj";         \
        try                                                         \
        {                                                           \
            meshes_[std::to_underlying(BuiltinMesh::NAME)] =        \
                ToRC(MeshManager::Load(device_, filename, flags));  \
        }                                                           \
        catch(...)                                                  \
        {                                                           \
            LogWarning("Fail to load builtin mesh {}", filename);   \
        }                                                           \
    }())
    LOAD_BUILTIN_MESH(Cube);

#undef LOAD_BUILTIN_MESH
}

void ResourceManager::LoadBuiltinTextures()
{
    textures_[std::to_underlying(BuiltinTexture::Black2D)] =
        device_->CreateColorTexture2D(0, 0, 0, 255, "BuiltinBlack2D");
    textures_[std::to_underlying(BuiltinTexture::White2D)] =
        device_->CreateColorTexture2D(255, 255, 255, 255, "BuiltinWhite2D");

    constexpr auto blueNoiseFilename = "Asset/Builtin/Texture/BlueNoise256.png";
    if(std::filesystem::is_regular_file(blueNoiseFilename))
    {
        auto blueNoise256 = Image<uint8_t>::Load(blueNoiseFilename);
        textures_[std::to_underlying(BuiltinTexture::BlueNoise256)] =
            device_->CreateAndUploadTexture2D(
                RHI::TextureDesc
                {
                    .dim                  = RHI::TextureDimension::Tex2D,
                    .format               = RHI::Format::R8_UNorm,
                    .width                = blueNoise256.GetWidth(),
                    .height               = blueNoise256.GetHeight(),
                    .depth                = 1,
                    .mipLevels            = 1,
                    .sampleCount          = 1,
                    .usage                = RHI::TextureUsage::ShaderResource,
                    .initialLayout        = RHI::TextureLayout::Undefined,
                    .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
                }, blueNoise256.GetData(), RHI::TextureLayout::ShaderTexture);
    }
}

void ResourceManager::LoadBuiltinMaterials()
{
    if(!std::filesystem::is_directory("Asset/Builtin/Material"))
    {
        return;
    }
    materialManager_.AddFiles($rtrc_get_files("Asset/Builtin/Material/*/*.*"));
    materialManager_.AddIncludeDirectory("Asset/Builtin/Material");

#define LOAD_BUILTIN_MATERIAL(NAME) \
    materials_[std::to_underlying(BuiltinMaterial::NAME)] = materialManager_.GetMaterial("Builtin/" #NAME)

    LOAD_BUILTIN_MATERIAL(DeferredLighting);
    LOAD_BUILTIN_MATERIAL(Atmosphere);
    LOAD_BUILTIN_MATERIAL(ShadowMask);
    LOAD_BUILTIN_MATERIAL(PathTracing);
    LOAD_BUILTIN_MATERIAL(KajiyaGI);

#undef LOAD_BUILTIN_MATERIAL
}

RTRC_END
