#include <Rtrc/Graphics/Mesh/MeshLoader.h>
#include <Rtrc/Renderer/BuiltinResources.h>

RTRC_BEGIN

BuiltinResourceManager::BuiltinResourceManager(Device &device)
    : device_(device)
{
    LoadBuiltinMaterials();
    LoadBuiltinTextures();
    LoadBuiltinMeshes();
}

const RC<Shader> &BuiltinResourceManager::GetBuiltinShader(BuiltinShader shader) const
{
    return shaders_[EnumToInt(shader)];
}

const RC<Mesh> &BuiltinResourceManager::GetBuiltinMesh(BuiltinMesh mesh) const
{
    return meshes_[EnumToInt(mesh)];
}

const RC<Texture> &BuiltinResourceManager::GetBuiltinTexture(BuiltinTexture texture) const
{
    return textures_[EnumToInt(texture)];
}

void BuiltinResourceManager::LoadBuiltinMaterials()
{
    materialManager_.SetDebugMode(RTRC_DEBUG);
    materialManager_.SetDevice(&device_);
    materialManager_.SetRootDirectory("./Asset/Builtin/Material/");

#define LOAD_BUILTIN_SHADER(NAME) \
    shaders_[EnumToInt(BuiltinShader::NAME)] = materialManager_.GetShader("Builtin/" #NAME)

    LOAD_BUILTIN_SHADER(DeferredLighting);

#undef LOAD_BUILTIN_SHADER
}

void BuiltinResourceManager::LoadBuiltinTextures()
{
    {
        auto tex = device_.CreateTexture(RHI::TextureDesc
        {
            .dim                  = RHI::TextureDimension::Tex2D,
            .format               = RHI::Format::B8G8R8A8_UNorm,
            .width                = 1,
            .height               = 1,
            .arraySize            = 1,
            .mipLevels            = 1,
            .sampleCount          = 1,
            .usage                = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::TransferDst,
            .initialLayout        = RHI::TextureLayout::Undefined,
            .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Concurrent
        });
        const Vector4b &data = Vector4b(0, 0, 0, 255);
        device_.GetCopyContext().UploadTexture2D(tex, 0, 0, &data);
        textures_[EnumToInt(BuiltinTexture::Black2D)] = std::move(tex);
    }

    {
        auto tex = device_.CreateTexture(RHI::TextureDesc
        {
            .dim                  = RHI::TextureDimension::Tex2D,
            .format               = RHI::Format::B8G8R8A8_UNorm,
            .width                = 1,
            .height               = 1,
            .arraySize            = 1,
            .mipLevels            = 1,
            .sampleCount          = 1,
            .usage                = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::TransferDst,
            .initialLayout        = RHI::TextureLayout::Undefined,
            .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Concurrent
        });
        const Vector4b &data = Vector4b(255, 255, 255, 255);
        device_.GetCopyContext().UploadTexture2D(tex, 0, 0, &data);
        textures_[EnumToInt(BuiltinTexture::White2D)] = std::move(tex);
    }

    device_.ExecuteAndWait([&](CommandBuffer &cmd)
    {
        cmd.ExecuteBarriers(BarrierBatch()
            (textures_[EnumToInt(BuiltinTexture::Black2D)],
             RHI::TextureLayout::CopyDst, RHI::PipelineStage::None, RHI::ResourceAccess::None,
             RHI::TextureLayout::ShaderTexture, RHI::PipelineStage::None, RHI::ResourceAccess::None)
            (textures_[EnumToInt(BuiltinTexture::White2D)],
             RHI::TextureLayout::CopyDst, RHI::PipelineStage::None, RHI::ResourceAccess::None,
             RHI::TextureLayout::ShaderTexture, RHI::PipelineStage::None, RHI::ResourceAccess::None));
    });
}

void BuiltinResourceManager::LoadBuiltinMeshes()
{
    MeshLoader meshManager;
    meshManager.SetCopyContext(&device_.GetCopyContext());
    meshManager.SetRootDirectory("./Asset/Builtin/Mesh/");

#define LOAD_BUILTIN_MESH(NAME) \
    meshes_[EnumToInt(BuiltinMesh::NAME)] = ToRC(meshManager.LoadFromObjFile(#NAME ".obj"))

    LOAD_BUILTIN_MESH(Cube);

#undef LOAD_BUILTIN_MESH
}

RTRC_END
