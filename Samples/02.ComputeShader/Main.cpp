#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

rtrc_group(MainGroup)
{
    rtrc_struct(ScaleSetting)
    {
        rtrc_var(float, scale) = 0;
    };
    rtrc_define(Texture2D<float4>,            InputTexture);
    rtrc_define(RWTexture2D<float4>,          OutputTexture);
    rtrc_define(ConstantBuffer<ScaleSetting>, ScaleSetting);
};

void Run()
{
    auto instance = RHI::CreateVulkanInstance({});
    RenderContext renderContext(instance->CreateDevice({ false, true, true, false }));

    MaterialManager materialManager;
    materialManager.SetRenderContext(&renderContext);
    materialManager.SetRootDirectory("Asset/02.ComputeShader/");

    KeywordValueContext keywords;

    auto material = materialManager.GetMaterial("ScaleImage");
    auto subMaterial = material->GetSubMaterialByTag("Default");
    auto shader = subMaterial->GetShader(keywords);
    auto pipeline = renderContext.CreateComputePipeline(shader);

    auto matInst = material->CreateInstance();
    auto subMatInst = matInst->GetSubMaterialInstance("Default");

    auto inputTexture = renderContext.GetCopyContext().LoadTexture2D(
        "Asset/01.TexturedQuad/MainTexture.png", RHI::Format::B8G8R8A8_UNorm, RHI::TextureUsage::ShaderResource, false);
    auto inputTextureSRV = inputTexture->GetSRV();

    auto outputTexture = renderContext.CreateTexture2D(
        inputTexture->GetWidth(),
        inputTexture->GetHeight(),
        1, 1, RHI::Format::B8G8R8A8_UNorm,
        RHI::TextureUsage::TransferSrc | RHI::TextureUsage::UnorderAccess,
        RHI::QueueConcurrentAccessMode::Exclusive,
        1, true);
    auto outputTextureUAV = outputTexture->GetUAV();

    auto readbackBuffer = renderContext.CreateBuffer(
        inputTexture->GetWidth() * inputTexture->GetHeight() * 4,
        RHI::BufferUsage::TransferDst,
        RHI::BufferHostAccessType::Random,
        false);

    const int bindingGroupIndex = shader->GetBindingGroupIndexByName("MainGroup");

    MainGroup bindingGroupValue;
    bindingGroupValue.InputTexture = inputTexture;
    bindingGroupValue.OutputTexture = outputTexture;
    bindingGroupValue.ScaleSetting.scale = 2.0f;
    auto bindingGroup = renderContext.CreateBindingGroup(bindingGroupValue);

    renderContext.ExecuteAndWaitImmediate([&](CommandBuffer &cmd)
    {
        cmd.ExecuteBarriers(BarrierBatch()
            (inputTexture, RHI::TextureLayout::ShaderTexture, RHI::PipelineStage::None, RHI::ResourceAccess::None)
            (outputTexture, RHI::TextureLayout::ShaderRWTexture, RHI::PipelineStage::None, RHI::ResourceAccess::None));

        cmd.BindPipeline(pipeline);
        cmd.BindComputeSubMaterial(subMatInst, keywords);
        cmd.BindComputeGroup(bindingGroupIndex, bindingGroup);

        constexpr Vector2i GROUP_SIZE = Vector2i(8, 8);
        const Vector2i groupCount = {
            (static_cast<int>(inputTexture->GetWidth()) + GROUP_SIZE.x - 1) / GROUP_SIZE.x,
            (static_cast<int>(inputTexture->GetHeight()) + GROUP_SIZE.y - 1) / GROUP_SIZE.y
        };
        cmd.Dispatch(groupCount.x, groupCount.y, 1);

        cmd.ExecuteBarriers(BarrierBatch()
            (outputTexture, RHI::TextureLayout::CopySrc, RHI::PipelineStage::Copy, RHI::ResourceAccess::CopyRead));
        cmd.CopyColorTexture2DToBuffer(*readbackBuffer, 0, *outputTexture, 0, 0);
    });
    readbackBuffer->SetUnsyncAccess(UnsynchronizedBufferAccess::Create(
        RHI::PipelineStage::None, RHI::ResourceAccess::None));

    Image<Vector4b> outputImageData(inputTexture->GetWidth(), inputTexture->GetHeight());
    readbackBuffer->Download(outputImageData.GetData(), 0, readbackBuffer->GetSize());
    for(uint32_t y = 0; y < outputImageData.GetHeight(); ++y)
    {
        for(uint32_t x = 0; x < outputImageData.GetWidth(); ++x)
        {
            auto &p = outputImageData(x, y);
            p = Vector4b(p.z, p.y, p.x, p.w);
        }
    }

    outputImageData.Save("./Asset/02.ComputeShader/Output.png");
}

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        Run();
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
