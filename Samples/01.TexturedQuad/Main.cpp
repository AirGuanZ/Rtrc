#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

rtrc_group(TexturedQuadBindingGroup)
{
    rtrc_define(Texture2D, MainTexture);
    rtrc_define(SamplerState, MainSampler);
    rtrc_uniform(float, scale);
    rtrc_uniform(float, mipLevel);
};

void Run()
{
    // Render context

    auto window = WindowBuilder()
        .SetSize(800, 800)
        .SetTitle("Rtrc Sample: TexturedQuad")
        .Create();

    auto device = Device::CreateGraphicsDevice({ .window = window });

    ResourceManager resourceManager(device);
    
    // Mesh

    auto mesh = resourceManager.GetMesh("Asset/Sample/01.TexturedQuad/Quad.obj");

    // Pipeline

    FastKeywordContext keywords;
    keywords.Set(RTRC_FAST_KEYWORD(DADADA), 1);

    auto pipeline = device->CreateGraphicsPipeline({
        .shader = device->GetShaderTemplate("Sample01/Quad", true)->GetVariant(keywords),
        .meshLayout = mesh->GetLayout(),
        .attachmentState = RTRC_ATTACHMENT_STATE
        {
            .colorAttachmentFormats = { device->GetSwapchainImageDesc().format }
        }
    });

    // Main texture

    auto mainTex = device->LoadTexture2D(
        "Asset/Sample/01.TexturedQuad/MainTexture.png",
        RHI::Format::B8G8R8A8_UNorm,
        RHI::TextureUsage::ShaderResource,
        true, RHI::TextureLayout::ShaderTexture);
    mainTex->SetName("MainTexture");

    // Main sampler

    auto mainSampler = device->CreateSampler(RHI::SamplerDesc
    {
        .magFilter = RHI::FilterMode::Linear,
        .minFilter = RHI::FilterMode::Linear,
        .mipFilter = RHI::FilterMode::Point,
        .addressModeU = RHI::AddressMode::Clamp,
        .addressModeV = RHI::AddressMode::Clamp,
        .addressModeW = RHI::AddressMode::Clamp
    });
    mainSampler->SetName("MainSampler");

    // Binding group

    TexturedQuadBindingGroup bindingGroupData;
    bindingGroupData.MainTexture = mainTex;
    bindingGroupData.MainSampler = mainSampler;
    bindingGroupData.scale = 1;
    bindingGroupData.mipLevel = 0;
    auto bindingGroup = device->CreateBindingGroup(bindingGroupData);

    window.SetFocus();

    // Render loop

    RGExecuter executer(device);

    device->BeginRenderLoop();
    while(!window.ShouldClose())
    {
        if(!device->BeginFrame())
        {
            continue;
        }
        executer.Recycle();

        if(window.GetInput().IsKeyDown(KeyCode::Escape))
        {
            window.SetCloseFlag(true);
        }

        auto graph = device->CreateRenderGraph();
        auto renderTarget = graph->RegisterSwapchainTexture(device->GetSwapchain());

        RGAddRenderPass(
            graph, "DrawQuad", RGColorAttachment
            {
                .rtv        = renderTarget->GetRtv(),
                .loadOp     = RHI::AttachmentLoadOp::Clear,
                .clearValue = RHI::ColorClearValue{ 0, 1, 1, 1 }
            },
            [&]
            {
                auto &commandBuffer = RGGetCommandBuffer();
                commandBuffer.BindGraphicsPipeline(pipeline);
                mesh->Bind(commandBuffer);
                commandBuffer.BindGraphicsGroup(0, bindingGroup);
                commandBuffer.SetViewports(renderTarget->GetViewport());
                commandBuffer.SetScissors(renderTarget->GetScissor());
                commandBuffer.DrawIndexed(6, 1, 0, 0, 0);
            });
        
        graph->SetCompleteFence(device->GetFrameFence());
        executer.Execute(graph);
        
        if(!device->Present())
        {
            window.SetCloseFlag(true);
        }
    }
    device->EndRenderLoop();
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
        std::cerr << e.what() << std::endl;
    }
}
