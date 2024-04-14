#include <Rtrc/Core/Math/LowDiscrepancy.h>
#include <Rtrc/Core/Resource/Image.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/ToolKit/Resource/ResourceManager.h>

RTRC_BEGIN

ResourceManager::ResourceManager(Ref<Device> device)
    : device_(device)
{
    LoadBuiltinTextures();
    GeneratePoissonDiskSamples();
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
        textures_[std::to_underlying(BuiltinTexture::BlueNoise256)] = device_->CreateAndUploadTexture(
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
    const auto data = Rtrc::GeneratePoissonDiskSamples<Vector2f>(2048, 42);
    poissonDiskSamples2048_ = device_->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size = sizeof(Vector2f) * 2048,
        .usage = RHI::BufferUsage::ShaderStructuredBuffer
    }, data.data());
    poissonDiskSamples2048_->SetName("MultiScatterDirSamples2048");
    poissonDiskSamples2048_->SetDefaultStructStride(sizeof(Vector2f));
}

RTRC_END
