#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

void Run()
{
    // Render context

    auto window = WindowBuilder()
        .SetSize(800, 800)
        .SetTitle("Rtrc Sample: TexturedQuad")
        .Create();

    auto instance = CreateVulkanInstance(RHI::VulkanInstanceDesc
    {
        .extensions = Window::GetRequiredVulkanInstanceExtensions()
    });

    auto device = instance->CreateDevice();
    RenderContext renderContext(device, &window);
    auto &copyContext = renderContext.GetCopyContext();

    // Mesh

    MeshManager meshManager(renderContext.GetCopyContext());
    auto mesh = meshManager.LoadFromObjFile("Asset/01.TexturedQuad/Quad.obj");

    // Pipeline

    MaterialManager materialManager;
    materialManager.SetRenderContext(&renderContext);
    materialManager.SetRootDirectory("Asset/01.TexturedQuad/");

    KeywordValueContext keywords;
    keywords.Set(RTRC_KEYWORD(DADADA), 1);

    auto material = materialManager.GetMaterial("Quad");
    auto subMaterial = material->GetSubMaterialByTag("Default");
    auto shader = subMaterial->GetShader(keywords);

    auto pipeline = renderContext.CreateGraphicsPipeline({
        .shader = shader,
        .meshLayout = mesh.GetLayout(),
        .colorAttachmentFormats = { renderContext.GetRenderTargetDesc().format }
    });

    // Main texture

    auto mainTex = copyContext.LoadTexture2D(
        "Asset/01.TexturedQuad/MainTexture.png",
        RHI::Format::B8G8R8A8_UNorm,
        RHI::TextureUsage::ShaderResource,
        true);
    mainTex->SetName("MainTexture");

    renderContext.ExecuteAndWaitImmediate([&](CommandBuffer &cmd)
    {
        cmd.ExecuteBarriers(BarrierBatch(
            mainTex, RHI::TextureLayout::ShaderTexture, RHI::PipelineStage::None, RHI::ResourceAccess::None));
    });

    // Main sampler

    auto mainSampler = renderContext.CreateSampler(RHI::SamplerDesc
    {
        .magFilter = RHI::FilterMode::Linear,
        .minFilter = RHI::FilterMode::Linear,
        .mipFilter = RHI::FilterMode::Point,
        .addressModeU = RHI::AddressMode::Clamp,
        .addressModeV = RHI::AddressMode::Clamp,
        .addressModeW = RHI::AddressMode::Clamp
    });
    mainSampler->SetName("MainSampler");

    // Material

    auto materialInstance = material->CreateInstance();
    auto subMaterialInstance = materialInstance->GetSubMaterialInstance(0);
    materialInstance->Set("MainTexture", mainTex);
    materialInstance->Set("MainSampler", mainSampler);
    materialInstance->SetFloat("scale", 1);
    materialInstance->SetFloat("mipLevel", 0);

    // Render loop

    RG::Executer executer(renderContext);

    renderContext.BeginRenderLoop();
    while(!window.ShouldClose())
    {
        if(!renderContext.BeginFrame())
        {
            continue;
        }

        if(window.GetInput().IsKeyDown(KeyCode::Escape))
        {
            window.SetCloseFlag(true);
        }

        auto graph = renderContext.CreateRenderGraph();
        
        auto renderTarget = graph->RegisterSwapchainTexture(renderContext.GetSwapchain());

        auto quadPass = graph->CreatePass("DrawQuad");
        quadPass->Use(renderTarget, RG::RENDER_TARGET);
        quadPass->SetCallback([&](RG::PassContext &context)
        {
            auto rt = renderTarget->Get();
            auto &commandBuffer = context.GetCommandBuffer();
            commandBuffer.BeginRenderPass(ColorAttachment
            {
                .renderTarget = rt,
                .loadOp       = AttachmentLoadOp::Clear,
                .storeOp      = AttachmentStoreOp::Store,
                .clearValue   = ColorClearValue{ 0, 1, 1, 1 }
            });
            commandBuffer.BindPipeline(pipeline);
            commandBuffer.SetMesh(mesh);
            commandBuffer.BindGraphicsSubMaterial(subMaterialInstance, keywords);
            commandBuffer.SetViewports(rt->GetViewport());
            commandBuffer.SetScissors(rt->GetScissor());
            commandBuffer.DrawIndexed(6, 1, 0, 0, 0);
            commandBuffer.EndRenderPass();
        });
        quadPass->SetSignalFence(renderContext.GetFrameFence());

        executer.Execute(graph);

        renderContext.Present();
    }
    renderContext.EndRenderLoop();
    renderContext.WaitIdle();
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
