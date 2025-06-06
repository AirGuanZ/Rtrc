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

    auto meshData = MeshData::LoadFromObjFile("Asset/Sample/01.TexturedQuad/Quad.obj");
    
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

    FastKeywordContext keywords;
    keywords.Set(RTRC_FAST_KEYWORD(DADADA), 1);

    auto pipeline = device->CreateGraphicsPipeline({
        .shader = device->GetShaderTemplate("Sample01/Quad", true)->GetVariant(keywords),
        .meshLayout = meshLayout,
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
                commandBuffer.SetVertexBuffers(0, { positionBuffer, uvBuffer }, { sizeof(Vector3f), sizeof(Vector2f) });
                commandBuffer.SetIndexBuffer(indexBuffer, RHI::IndexFormat::UInt32);
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
        LogError("{}\n{}", e.what(), e.stacktrace());
    }
    catch(const std::exception &e)
    {
        LogError(e.what());
        std::cerr << e.what() << std::endl;
    }
}
