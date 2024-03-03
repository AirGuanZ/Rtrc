#include <Rtrc/Rtrc.h>

using namespace Rtrc;

#include "Bindless.shader.outh"

void Run()
{
    auto window = WindowBuilder()
        .SetSize(800, 800)
        .SetTitle("Rtrc Sample: Bindless")
        .Create();

    auto device = Device::CreateGraphicsDevice({ .window = window });

    ResourceManager resourceManager(device);

    // Mesh

    auto mesh = resourceManager.GetMesh("Asset/Sample/03.BindlessTexture/Quad.obj");

    // Pipeline

    auto shader = device->GetShader("Bindless", true);
    auto pipeline = device->CreateGraphicsPipeline(
    {
        .shader = shader,
        .meshLayout = mesh->GetLayout(),
        .attachmentState = RTRC_ATTACHMENT_STATE
        {
            .colorAttachmentFormats = { device->GetSwapchainImageDesc().format }
        }
    });

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

    constexpr int BINDING_GROUP_SIZE = 2048;
    RangeSet bindingSlotAllocator(BINDING_GROUP_SIZE);

    StaticShaderInfo<"Bindless">::Pass bindingGroupData;
    bindingGroupData.alpha = 1;
    auto bindingGroup = device->CreateBindingGroup(bindingGroupData, BINDING_GROUP_SIZE);

    RangeSet::Index slots[4];
    for(int i = 0; i < 4; ++i)
    {
        slots[i] = bindingSlotAllocator.Allocate(1);
        bindingGroup->Set(1, static_cast<int>(slots[i]), textures[i]->GetSrv());
    }

    RGExecuter renderGraphExecuter(device);

    window.SetFocus();
    device->BeginRenderLoop();
    while(!window.ShouldClose())
    {
        if(!device->BeginFrame())
        {
            continue;
        }
        renderGraphExecuter.Recycle();

        if(window.GetInput().IsKeyDown(KeyCode::Escape))
        {
            window.SetCloseFlag(true);
        }

        auto graph = device->CreateRenderGraph();

        auto renderTarget = graph->RegisterSwapchainTexture(device->GetSwapchain());

        auto quadPass = graph->CreatePass("DrawQuads");
        quadPass->Use(renderTarget, RG::ColorAttachment);
        quadPass->SetCallback([&]
        {
            auto &commandBuffer = RGGetCommandBuffer();

            commandBuffer.BeginRenderPass(RHI::ColorAttachment
            {
                .renderTargetView = renderTarget->GetRtvImm(),
                .loadOp           = RHI::AttachmentLoadOp::Clear,
                .storeOp          = RHI::AttachmentStoreOp::Store,
                .clearValue       = { 0, 0, 0, 0 }
            });
            RTRC_SCOPE_EXIT{ commandBuffer.EndRenderPass(); };

            commandBuffer.BindGraphicsPipeline(pipeline);
            mesh->Bind(commandBuffer);
            commandBuffer.BindGraphicsGroup(0, bindingGroup);
            commandBuffer.SetViewports(renderTarget->GetViewport());
            commandBuffer.SetScissors(renderTarget->GetScissor());
            commandBuffer.SetGraphicsPushConstants(0, Vector4u(slots[0], slots[1], slots[2], slots[3]));
            commandBuffer.DrawIndexed(6, 1, 0, 0, 0);
        });
        
        graph->SetCompleteFence(device->GetFrameFence());
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
#if RTRC_ENABLE_EXCEPTION_STACKTRACE
        LogError("{}\n{}", e.what(), e.stacktrace());
#else
        LogError("{}", e.what());
#endif
    }
    catch(const std::exception &e)
    {
        LogError(e.what());
    }
}
