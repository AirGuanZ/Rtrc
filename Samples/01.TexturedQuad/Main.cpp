#include <iostream>

#include <Rtrc/Rtrc.h>

rtrc_group(TestGroup)
{
    rtrc_define(Texture2D<float2>, MainTexture);
    rtrc_define(SamplerState[4], MainSampler);
    rtrc_define(ConstantBuffer<float3>, MainConstantBuffer);
};

using namespace Rtrc;

void Run()
{
    // Render context

    auto window = WindowBuilder()
        .SetSize(800, 800)
        .SetTitle("Rtrc Sample: TexturedQuad")
        .Create();

    auto device = Device::CreateGraphicsDevice(window);

    // Mesh

    MeshManager meshManager(device->GetCopyContext());
    auto mesh = meshManager.LoadFromObjFile("Asset/01.TexturedQuad/Quad.obj");

    // Pipeline

    MaterialManager materialManager;
    materialManager.SetDevice(device.get());
    materialManager.SetRootDirectory("Asset/01.TexturedQuad/");

    KeywordValueContext keywords;
    keywords.Set(RTRC_KEYWORD(DADADA), 1);

    auto material = materialManager.GetMaterial("Quad");
    auto subMaterial = material->GetSubMaterialByTag("Default");
    auto shader = subMaterial->GetShader(keywords);

    auto pipeline = device->CreateGraphicsPipeline({
        .shader = shader,
        .meshLayout = mesh.GetLayout(),
        .colorAttachmentFormats = { device->GetSwapchainImageDesc().format }
    });

    // Main texture

    auto mainTex = device->GetCopyContext().LoadTexture2D(
        "Asset/01.TexturedQuad/MainTexture.png",
        RHI::Format::B8G8R8A8_UNorm,
        RHI::TextureUsage::ShaderResource,
        true);
    //mainTex->SetName("MainTexture");

    device->ExecuteAndWait([&](CommandBuffer &cmd)
    {
        cmd.ExecuteBarriers(BarrierBatch(
            mainTex, RHI::TextureLayout::ShaderTexture, RHI::PipelineStage::None, RHI::ResourceAccess::None));
    });

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
    //mainSampler->SetName("MainSampler");

    // Material

    auto materialInstance = material->CreateInstance();
    auto subMaterialInstance = materialInstance->GetSubMaterialInstance(0);
    materialInstance->Set("MainTexture", mainTex);
    materialInstance->Set("MainSampler", mainSampler);
    materialInstance->SetFloat("scale", 1);
    materialInstance->SetFloat("mipLevel", 0);

    // Render loop

    RG::Executer executer(device.get());

    device->BeginRenderLoop();
    while(!window.ShouldClose())
    {
        if(!device->BeginFrame())
        {
            continue;
        }

        if(window.GetInput().IsKeyDown(KeyCode::Escape))
        {
            window.SetCloseFlag(true);
        }

        auto graph = device->CreateRenderGraph();
        
        auto renderTarget = graph->RegisterSwapchainTexture(device->GetSwapchain());

        auto quadPass = graph->CreatePass("DrawQuad");
        quadPass->Use(renderTarget, RG::RENDER_TARGET);
        quadPass->SetCallback([&](RG::PassContext &context)
        {
            auto rt = renderTarget->Get();
            auto &commandBuffer = context.GetCommandBuffer();
            commandBuffer.BeginRenderPass(ColorAttachment
            {
                .renderTargetView = rt->CreateRTV(),
                .loadOp       = AttachmentLoadOp::Clear,
                .storeOp      = AttachmentStoreOp::Store,
                .clearValue   = ColorClearValue{ 0, 1, 1, 1 }
            });
            commandBuffer.BindPipeline(pipeline);
            commandBuffer.BindMesh(mesh);
            subMaterialInstance->BindGraphicsProperties(keywords, commandBuffer);
            commandBuffer.SetViewports(rt->GetViewport());
            commandBuffer.SetScissors(rt->GetScissor());
            commandBuffer.DrawIndexed(6, 1, 0, 0, 0);
            commandBuffer.EndRenderPass();
        });
        quadPass->SetSignalFence(device->GetFrameFence());

        executer.Execute(graph);

        device->Present();
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
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
