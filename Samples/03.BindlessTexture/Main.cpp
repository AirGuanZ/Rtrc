#include <Rtrc/Rtrc.h>

using namespace Rtrc;

rtrc_group(Pass)
{
    rtrc_uniform(float, alpha);
    rtrc_bindless_variable_size(Texture2D[2048], Textures);
};

void Run()
{
    auto window = WindowBuilder()
        .SetSize(800, 800)
        .SetTitle("Rtrc Sample: Bindless")
        .Create();

    auto device = Device::CreateGraphicsDevice(window);

    ResourceManager resourceManager(device);
    resourceManager.AddMaterialFiles($rtrc_get_files("Asset/Sample/03.BindlessTexture/*.*"));

    // Mesh

    auto mesh = resourceManager.GetMesh("Asset/Sample/03.BindlessTexture/Quad.obj");

    // Pipeline

    auto material = resourceManager.GetMaterial("Quad");
    auto shader = material->GetPassByIndex(0)->GetShader();
    auto pipeline = device->CreateGraphicsPipeline(
    {
        .shader = shader,
        .meshLayout = mesh->GetLayout(),
        .colorAttachmentFormats = { device->GetSwapchainImageDesc().format }
    });
    auto matInst = material->CreateInstance();
    auto matPassInst = matInst->GetPassInstance(0);

    // Textures

    RC<Texture> textures[4];
    for(int i = 0; i < 3; ++i)
    {
        const std::string filename = fmt::format("Asset/Sample/03.BindlessTexture/{}.png", i);
        textures[i] = device->LoadTexture2D(
            filename, RHI::Format::R8G8B8A8_UNorm,
            RHI::TextureUsage::TransferDst | RHI::TextureUsage::ShaderResource,
            false, RHI::TextureLayout::ShaderTexture);
    }
    textures[3] = device->LoadTexture2D(
        "Asset/Sample/01.TexturedQuad/MainTexture.png", RHI::Format::R8G8B8A8_UNorm,
        RHI::TextureUsage::TransferDst | RHI::TextureUsage::ShaderResource,
        false, RHI::TextureLayout::ShaderTexture);

    // Binding group

    constexpr int BINDING_GROUP_SIZE = 1024;
    RangeSet bindingSlotAllocator(BINDING_GROUP_SIZE);

    Pass bindingGroupData;
    bindingGroupData.alpha = 1;
    auto bindingGroup = device->CreateBindingGroup(bindingGroupData, BINDING_GROUP_SIZE);

    RangeSet::Index slots[4];
    for(int i = 0; i < 4; ++i)
    {
        slots[i] = bindingSlotAllocator.Allocate(1);
        bindingGroup->Set(1, slots[i], textures[i]->CreateSrv());
    }

    RG::Executer renderGraphExecuter(device);

    window.SetFocus();
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

        auto quadPass = graph->CreatePass("DrawQuads");
        quadPass->Use(renderTarget, RG::COLOR_ATTACHMENT);
        quadPass->SetCallback([&](RG::PassContext &context)
        {
            auto rt = renderTarget->Get(context);
            auto &commandBuffer = context.GetCommandBuffer();

            commandBuffer.BeginRenderPass(ColorAttachment
            {
                .renderTargetView = rt->CreateRtv(),
                .loadOp           = AttachmentLoadOp::Clear,
                .storeOp          = AttachmentStoreOp::Store,
                .clearValue       = ColorClearValue(0, 0, 0, 0)
            });
            RTRC_SCOPE_EXIT{ commandBuffer.EndRenderPass(); };

            commandBuffer.BindGraphicsPipeline(pipeline);
            commandBuffer.BindMesh(*mesh);
            matPassInst->BindGraphicsProperties(0, commandBuffer);
            commandBuffer.BindGraphicsGroup(0, bindingGroup);
            commandBuffer.SetViewports(rt->GetViewport());
            commandBuffer.SetScissors(rt->GetScissor());
            commandBuffer.SetGraphicsPushConstants(Vector4i(slots[0], slots[1], slots[2], slots[3]));
            commandBuffer.DrawIndexed(6, 1, 0, 0, 0);
        });

        quadPass->SetSignalFence(device->GetFrameFence());
        renderGraphExecuter.Execute(graph);

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
    catch(const Exception &e)
    {
        LogError("{}\n{}", e.what(), e.stacktrace());
    }
    catch(const std::exception &e)
    {
        LogError(e.what());
    }
}
