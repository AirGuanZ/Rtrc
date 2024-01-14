#include <Rtrc/Core/Math/LowDiscrepancy.h>
#include <Rtrc/Core/Resource/Image.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/ToolKit/Resource/ResourceManager.h>

RTRC_BEGIN

namespace ReflectedStruct
{

    const char *GetGeneratedFilePath();

} // namespace ReflectedStruct

ResourceManager::ResourceManager(Ref<Device> device, bool debugMode)
    : device_(device)
{
    meshManager_.SetDevice(device);
    shaderManager_.SetDevice(device);
    shaderManager_.SetDebug(debugMode);

    LoadBuiltinMeshes();
    LoadBuiltinTextures();
    GeneratePoissonDiskSamples();
}

RC<ShaderTemplate> ResourceManager::GetShaderTemplate(const std::string &name, bool persistent)
{
    return shaderManager_.GetShaderTemplate(name, persistent);
}

RC<Shader> ResourceManager::GetShader(const std::string &name, bool persistent)
{
    return shaderManager_.GetShader(name, persistent);
}

RC<Mesh> ResourceManager::GetMesh(std::string_view name, MeshFlags flags)
{
    return meshManager_.GetMesh(name, flags);
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
        textures_[std::to_underlying(BuiltinTexture::BlueNoise256)] = device_->CreateAndUploadTexture2D(
            RHI::TextureDesc
            {
                .format = RHI::Format::R8_UNorm,
                .width  = blueNoise256.GetWidth(),
                .height = blueNoise256.GetHeight(),
                .usage  = RHI::TextureUsage::ShaderResource,
            }, blueNoise256.GetData(), RHI::TextureLayout::ShaderTexture);
    }
}

void ResourceManager::GeneratePoissonDiskSamples()
{
    auto data = Rtrc::GeneratePoissonDiskSamples<Vector2f>(2048, 42);
    poissonDiskSamples2048_ = device_->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size = sizeof(Vector2f) * 2048,
        .usage = RHI::BufferUsage::ShaderStructuredBuffer
    }, data.data());
    poissonDiskSamples2048_->SetName("MultiScatterDirSamples2048");
    poissonDiskSamples2048_->SetDefaultStructStride(sizeof(Vector2f));
}

RTRC_END
