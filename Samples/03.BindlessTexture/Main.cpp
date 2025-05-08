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

    auto meshData = MeshData::LoadFromObjFile("Asset/Sample/03.BindlessTexture/Quad.obj");
    
    auto positionBuffer = device->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size = sizeof(Vector3f) * meshData.positionData.size(),
        .usage = RHI::BufferUsage::VertexBuffer
    }, meshData.positionData.data());
    auto uvBuffer = device->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size = sizeof(Vector2f) * meshData.texCoordData.size(),
        .usage = RHI::BufferUsage::VertexBuffer
    }, meshData.texCoordData.data());
    auto indexBuffer = device->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size = sizeof(uint32_t) * meshData.indexData.size(),
        .usage = RHI::BufferUsage::IndexBuffer
    }, meshData.indexData.data());

    auto meshLayout = RTRC_MESH_LAYOUT(Buffer(Attribute("POSITION", Float3)), Buffer(Attribute("TEXCOORD", Float2)));

    // Pipeline

    auto shader = device->GetShader("Bindless", true);
    auto pipeline = device->CreateGraphicsPipeline(
    {
        .shader = shader,
        .meshLayout = meshLayout,
        .attachmentState = RTRC_ATTACHMENT_STATE
        {
            .colorAttachmentFormats = { device->GetSwapchainImageDesc().format }
        }
    });

    // Textures

    RC<Texture> textures[4];
    for(int i = 0; i < 3; ++i)
    {
        const std::string filename = std::format("Asset/Sample/03.BindlessTexture/{}.png", i);
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
            commandBuffer.SetVertexBuffers(0, { positionBuffer, uvBuffer }, { sizeof(Vector3f), sizeof(Vector2f) });
            commandBuffer.SetIndexBuffer(indexBuffer, RHI::IndexFormat::UInt32);
            commandBuffer.BindGraphicsGroup(0, bindingGroup);
            commandBuffer.SetViewports(renderTarget->GetViewport());
            commandBuffer.SetScissors(renderTarget->GetScissor());
            commandBuffer.SetGraphicsPushConstants(0, Vector4u(slots[0], slots[1], slots[2], slots[3]));
            commandBuffer.DrawIndexed(6, 1, 0, 0, 0);
        });
        
        graph->SetCompleteFence(device->GetFrameFence());
        renderGraphExecuter.Execute(graph);

        device->Present();
        device->EndFrame();
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
