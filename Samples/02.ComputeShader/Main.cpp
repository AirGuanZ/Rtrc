#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

rtrc_group(MainGroup)
{
    rtrc_struct(ScaleSetting)
    {
        rtrc_var(float, scale) = 0;
    };
    rtrc_define(Texture2D,                    InputTexture);
    rtrc_define(RWTexture2D,                  OutputTexture);
    rtrc_define(ConstantBuffer<ScaleSetting>, ScaleSetting);
};

void Run()
{
    auto device = Device::CreateComputeDevice();

    ResourceManager resourceManager(device);
    resourceManager.AddMaterialFiles($rtrc_get_files("Asset/Sample/02.ComputeShader/*.*"));
    
    KeywordContext keywords;

    auto material = resourceManager.GetMaterial("ScaleImage");
    auto matPass = material->GetPassByTag(RTRC_MATERIAL_PASS_TAG(Default));
    auto shader = matPass->GetShader(keywords);
    auto pipeline = shader->GetComputePipeline();

    auto matInst = material->CreateInstance();
    auto matPassInst = matInst->GetPassInstance(RTRC_MATERIAL_PASS_TAG(Default));

    auto inputTexture = device->LoadTexture2D(
        "Asset/Sample/01.TexturedQuad/MainTexture.png", RHI::Format::B8G8R8A8_UNorm, 
        RHI::TextureUsage::ShaderResource, false, RHI::TextureLayout::ShaderTexture);
    auto inputTextureSrv = inputTexture->CreateSrv();

    auto outputTexture = StatefulTexture::FromTexture(device->CreateTexture(RHI::TextureDesc
        {
            .dim = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::B8G8R8A8_UNorm,
            .width = inputTexture->GetDesc().width,
            .height = inputTexture->GetDesc().height,
            .arraySize = 1,
            .mipLevels = 1,
            .sampleCount = 1,
            .usage = RHI::TextureUsage::TransferSrc | RHI::TextureUsage::UnorderAccess,
            .initialLayout = RHI::TextureLayout::Undefined,
            .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
        }));
    auto outputTextureUav = outputTexture->CreateUav();

    auto readbackBuffer = device->CreateBuffer(RHI::BufferDesc
        {
            .size = inputTexture->GetDesc().width * inputTexture->GetDesc().height * 4,
            .usage = RHI::BufferUsage::TransferDst,
            .hostAccessType = RHI::BufferHostAccessType::Random
        });

    const int bindingGroupIndex = shader->GetBindingGroupIndexByName("MainGroup");

    MainGroup bindingGroupValue;
    bindingGroupValue.InputTexture = inputTexture;
    bindingGroupValue.OutputTexture = outputTexture;
    bindingGroupValue.ScaleSetting.scale = 2.0f;
    auto bindingGroup = device->CreateBindingGroup(bindingGroupValue);

    device->ExecuteAndWait([&](CommandBuffer &cmd)
    {
        cmd.ExecuteBarrier(
            outputTexture,
            RHI::TextureLayout::ShaderRWTexture,
            RHI::PipelineStage::ComputeShader,
            RHI::ResourceAccess::RWTextureWrite);

        cmd.BindComputePipeline(pipeline);
        matPassInst->BindGraphicsProperties(keywords, cmd);
        cmd.BindComputeGroup(bindingGroupIndex, bindingGroup);

        constexpr Vector2i GROUP_SIZE = Vector2i(8, 8);
        const Vector2i groupCount = {
            (static_cast<int>(inputTexture->GetDesc().width) + GROUP_SIZE.x - 1) / GROUP_SIZE.x,
            (static_cast<int>(inputTexture->GetDesc().height) + GROUP_SIZE.y - 1) / GROUP_SIZE.y
        };
        cmd.Dispatch(groupCount.x, groupCount.y, 1);

        cmd.ExecuteBarrier(
            outputTexture,
            RHI::TextureLayout::CopySrc,
            RHI::PipelineStage::Copy, 
            RHI::ResourceAccess::CopyRead);
        cmd.CopyColorTexture2DToBuffer(*readbackBuffer, 0, *outputTexture, 0, 0);
    });
    
    Image<Vector4b> outputImageData(inputTexture->GetDesc().width, inputTexture->GetDesc().height);
    readbackBuffer->Download(outputImageData.GetData(), 0, readbackBuffer->GetSize());
    for(uint32_t y = 0; y < outputImageData.GetHeight(); ++y)
    {
        for(uint32_t x = 0; x < outputImageData.GetWidth(); ++x)
        {
            auto &p = outputImageData(x, y);
            p = Vector4b(p.z, p.y, p.x, p.w);
        }
    }

    outputImageData.Save("./Asset/Sample/02.ComputeShader/Output.png");
}

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        Run();
    }
    catch(const Exception &e)
    {
        LogError("{}\n{}", e.what(), e.stacktrace());
    }
    catch(const std::exception &e)
    {
        LogError(e.what());
    }
}
